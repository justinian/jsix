#include <j6/signals.h>
#include <util/pointers.h>

#include "cpu.h"
#include "logger.h"
#include "memory.h"
#include "objects/thread.h"
#include "objects/process.h"
#include "objects/vm_area.h"
#include "scheduler.h"

extern "C" void kernel_to_user_trampoline();
static constexpr j6_signal_t thread_default_signals = 0;

extern vm_area_guarded &g_kernel_stacks;

thread::thread(process &parent, uint8_t pri, uintptr_t rsp0) :
    kobject(kobject::type::thread, thread_default_signals),
    m_parent(parent),
    m_state(state::loading),
    m_wait_type(wait_type::none),
    m_wait_data(0),
    m_wait_obj(0)
{
    parent.space().initialize_tcb(m_tcb);
    m_tcb.priority = pri;
    m_tcb.thread = this;

    if (!rsp0)
        setup_kernel_stack();
    else
        m_tcb.rsp0 = rsp0;

    m_creator = current_cpu().thread;
    m_self_handle = parent.add_handle(this);
}

thread::~thread()
{
    g_kernel_stacks.return_section(m_tcb.kernel_stack);
}

thread & thread::current() { return *current_cpu().thread; }

inline void schedule_if_current(thread *t) { if (t == current_cpu().thread) scheduler::get().schedule(); }

void
thread::wait_on_signals(j6_signal_t signals)
{
    {
        util::scoped_lock {m_wait_lock};
        m_wait_type = wait_type::signal;
        m_wait_data = signals;
        clear_state(state::ready);
    }
    schedule_if_current(this);
}

void
thread::wait_on_time(uint64_t t)
{
    {
        util::scoped_lock {m_wait_lock};
        m_wait_type = wait_type::time;
        m_wait_time = t;
        clear_state(state::ready);
    }
    schedule_if_current(this);
}

void
thread::wait_on_object(kobject *o, uint64_t t)
{
    {
        util::scoped_lock {m_wait_lock};

        m_wait_type = wait_type::object;
        m_wait_data = reinterpret_cast<uint64_t>(o);

        if (t) {
            m_wait_type |= wait_type::time;
            m_wait_time = t;
        }

        clear_state(state::ready);
    }
    schedule_if_current(this);
}

bool
thread::wake_on_signals(kobject *obj, j6_signal_t signals)
{
    util::scoped_lock {m_wait_lock};

    if (!(m_wait_type & wait_type::signal) ||
        (signals & m_wait_data) == 0)
        return false;

    m_wait_type = wait_type::none;
    m_wait_result = j6_status_ok;
    m_wait_data = signals;
    m_wait_obj = obj->koid();
    set_state(state::ready);
    return true;
}

bool
thread::wake_on_time(uint64_t now)
{
    util::scoped_lock {m_wait_lock};

    if (!(m_wait_type & wait_type::time) ||
        now < m_wait_time)
        return false;

    if (!(m_wait_type & ~wait_type::none))
        m_wait_result = j6_status_ok;
    else
        m_wait_result = j6_err_timed_out;

    m_wait_type = wait_type::none;
    m_wait_data = now;
    m_wait_obj = 0;
    set_state(state::ready);
    return true;
}

bool
thread::wake_on_object(kobject *o)
{
    util::scoped_lock {m_wait_lock};

    if (!(m_wait_type & wait_type::object) ||
        reinterpret_cast<uint64_t>(o) != m_wait_data)
        return false;

    m_wait_type = wait_type::none;
    m_wait_result = j6_status_ok;
    m_wait_obj = o->koid();
    set_state(state::ready);
    return true;
}

void
thread::wake_on_result(kobject *obj, j6_status_t result)
{
    util::scoped_lock {m_wait_lock};

    m_wait_type = wait_type::none;
    m_wait_result = result;
    m_wait_data = 0;
    m_wait_obj = obj->koid();
    set_state(state::ready);
}

void
thread::exit(int32_t code)
{
    m_return_code = code;
    set_state(state::exited);
    clear_state(state::ready);
    close();

    schedule_if_current(this);
}

void
thread::add_thunk_kernel(uintptr_t rip)
{
    // This adds just enough values to the top of the
    // kernel stack to come out of task_switch correctly
    // and start executing at rip (still in kernel mode)

    m_tcb.rsp -= sizeof(uintptr_t) * 7;
    uintptr_t *stack = reinterpret_cast<uintptr_t*>(m_tcb.rsp);

    stack[6] = rip;        // return rip
    stack[5] = m_tcb.rsp0; // rbp
    stack[4] = 0xbbbbbbbb; // rbx
    stack[3] = 0x12121212; // r12
    stack[2] = 0x13131313; // r13
    stack[1] = 0x14141414; // r14
    stack[0] = 0x15151515; // r15
}

void
thread::add_thunk_user(uintptr_t rip3, uintptr_t rip0, uint64_t flags)
{
    // This sets up the stack to:
    // a) come out of task_switch and return to rip0 (default is the
    //    kernel/user trampoline) (via add_thunk_kernel) - if this is
    //    changed, it needs to end up at the trampoline with the stack
    //    as it was
    // b) come out of the kernel/user trampoline and start executing
    //    in user mode at rip

    flags |= 0x200;
    m_tcb.rflags3 = flags;

    m_tcb.rsp -= sizeof(uintptr_t) * 7;
    uintptr_t *stack = reinterpret_cast<uintptr_t*>(m_tcb.rsp);

    stack[6] = rip3;       // return rip in rcx
    stack[5] = m_tcb.rsp3; // rbp
    stack[4] = 0xbbbbbbbb; // rbx
    stack[3] = 0x12121212; // r12
    stack[2] = 0x13131313; // r13
    stack[1] = 0x14141414; // r14
    stack[0] = 0x15151515; // r15

    static const uintptr_t trampoline =
        reinterpret_cast<uintptr_t>(kernel_to_user_trampoline);
    add_thunk_kernel(rip0 ? rip0 : trampoline);
}

void
thread::setup_kernel_stack()
{
    using mem::frame_size;
    using mem::kernel_stack_pages;
    static constexpr size_t stack_bytes = kernel_stack_pages * frame_size;

    constexpr unsigned null_frame_entries = 2;
    constexpr size_t null_frame_size = null_frame_entries * sizeof(uint64_t);

    uintptr_t stack_addr = g_kernel_stacks.get_section();
    uintptr_t stack_end = stack_addr + stack_bytes;

    uint64_t *null_frame = reinterpret_cast<uint64_t*>(stack_end - null_frame_size);
    for (unsigned i = 0; i < null_frame_entries; ++i)
        null_frame[i] = 0;

    log::debug(logs::memory, "Created kernel stack at %016lx size 0x%lx",
            stack_addr, stack_bytes);

    m_tcb.kernel_stack = stack_addr;
    m_tcb.rsp0 = reinterpret_cast<uintptr_t>(null_frame);
    m_tcb.rsp = m_tcb.rsp0;
}

thread *
thread::create_idle_thread(process &kernel, uint8_t pri, uintptr_t rsp0)
{
    thread *idle = new thread(kernel, pri, rsp0);
    idle->set_state(state::constant);
    idle->set_state(state::ready);
    return idle;
}
