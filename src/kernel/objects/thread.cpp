#include <util/pointers.h>

#include "kassert.h"
#include "capabilities.h"
#include "cpu.h"
#include "logger.h"
#include "memory.h"
#include "objects/thread.h"
#include "objects/process.h"
#include "objects/vm_area.h"
#include "scheduler.h"
#include "xsave.h"

extern "C" void initialize_user_cpu();
extern obj::vm_area_guarded &g_kernel_stacks;


namespace obj {

thread::thread(process &parent, uint8_t pri, uintptr_t rsp0) :
    kobject  {kobject::type::thread},
    m_parent {parent},
    m_state  {state::none},
    m_wake_value   {0},
    m_wake_timeout {0}
{
    parent.handle_retain();
    parent.space().initialize_tcb(m_tcb);
    m_tcb.priority = pri;
    m_tcb.thread = this;

    if (!rsp0)
        setup_kernel_stack();
    else
        m_tcb.rsp0 = rsp0;

    m_creator = current_cpu().thread;

    asm volatile ( "stmxcsr %0" : "=m"(m_mxcsr) );
    m_mxcsr
        .clear(mxcsr::IE)
        .clear(mxcsr::DE)
        .clear(mxcsr::ZE)
        .clear(mxcsr::OE)
        .clear(mxcsr::UE)
        .clear(mxcsr::PE);
}

thread::~thread()
{
    if (m_tcb.xsave)
        delete [] reinterpret_cast<uint8_t*>(m_tcb.xsave);

    g_kernel_stacks.return_section(m_tcb.kernel_stack);
    m_parent.handle_release();
}

thread & thread::current() { return *current_cpu().thread; }

uint64_t
thread::block()
{
    clear_state(state::ready);
    if (current_cpu().thread == this)
        scheduler::get().schedule();
    return m_wake_value;
}

uint64_t
thread::block(util::scoped_lock &lock)
{
    kassert(current_cpu().thread == this,
            "unlocking block() called on non-current thread");

    clear_state(state::ready);
    lock.release();
    scheduler::get().schedule();
    return m_wake_value;
}

j6_status_t
thread::join()
{
    if (has_state(state::exited))
        return j6_status_ok;

    thread &caller = current();
    if (&caller == this)
        return j6_err_invalid_arg;

    m_join_queue.add_thread(&caller);
    return caller.block();
}

void
thread::wake(uint64_t value)
{
    if (has_state(state::ready))
        return;

    m_wake_value = value;
    wake_only();
    scheduler::get().maybe_schedule(tcb());
}

void
thread::wake_only()
{
    m_wake_timeout = 0;
    set_state(state::ready);
}

void
thread::exit()
{
    m_wake_timeout = 0;
    set_state(state::exited);
    m_parent.thread_exited(this);
    m_join_queue.clear();
    block();
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
thread::add_thunk_user(uintptr_t rip3, uint64_t arg0, uint64_t arg1, uintptr_t rip0, uint64_t flags)
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

    m_tcb.rsp -= sizeof(uintptr_t) * 9;
    uintptr_t *stack = reinterpret_cast<uintptr_t*>(m_tcb.rsp);

    stack[8] = rip3;       // return rip in rcx
    stack[7] = m_tcb.rsp3; // rbp
    stack[6] = 0xbbbbbbbb; // rbx
    stack[5] = 0x12121212; // r12
    stack[4] = 0x13131313; // r13
    stack[3] = 0x14141414; // r14
    stack[2] = 0x15151515; // r15

    stack[1] = arg1;       // rsi
    stack[0] = arg0;       // rdi

    static const uintptr_t trampoline =
        reinterpret_cast<uintptr_t>(initialize_user_cpu);
    add_thunk_kernel(rip0 ? rip0 : trampoline);
}

void
thread::init_xsave_area()
{
    void *xsave_area = new uint8_t [xsave_size];
    memset(xsave_area, 0, xsave_size);
    m_tcb.xsave = reinterpret_cast<uintptr_t>(xsave_area);
}

void
thread::setup_kernel_stack()
{
    using mem::frame_size;
    using mem::kernel_stack_pages;
    static constexpr size_t stack_bytes = kernel_stack_pages * frame_size;

    static constexpr unsigned null_frame_entries = 2;
    static constexpr size_t null_frame_size = null_frame_entries * sizeof(uint64_t);

    uintptr_t stack_addr = g_kernel_stacks.get_section();
    uintptr_t stack_end = stack_addr + stack_bytes;

    uint64_t *null_frame = reinterpret_cast<uint64_t*>(stack_end - null_frame_size);
    for (unsigned i = 0; i < null_frame_entries; ++i)
        null_frame[i] = 0;

    log::verbose(logs::memory, "Created kernel stack at %016lx size 0x%lx",
            stack_addr, stack_bytes);

    m_tcb.kernel_stack = stack_addr;
    m_tcb.rsp0 = reinterpret_cast<uintptr_t>(null_frame);
    m_tcb.rsp = m_tcb.rsp0;
}

thread *
thread::create_idle_thread(process &kernel, uintptr_t rsp0)
{
    thread *idle = new thread(kernel, scheduler::idle_priority, rsp0);
    idle->set_state(state::constant);
    idle->set_state(state::ready);
    return idle;
}

} // namespace obj

uint32_t
__current_thread_id()
{
    cpu_data &cpu = current_cpu();
    return cpu.thread ? cpu.thread->obj_id() : -1u;
}
