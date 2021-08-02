#include "kutil/assert.h"
#include "kutil/no_construct.h"
#include "cpu.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "scheduler.h"

// This object is initialized _before_ global constructors are called,
// so we don't want it to have a global constructor at all, lest it
// overwrite the previous initialization.
static kutil::no_construct<process> __g_kernel_process_storage;
process &g_kernel_process = __g_kernel_process_storage.value;


process::process() :
    kobject {kobject::type::process},
    m_next_handle {1},
    m_state {state::running}
{
    j6_handle_t self = add_handle(this);
    kassert(self == self_handle(), "Process self-handle is not 1");
}

// The "kernel process"-only constructor
process::process(page_table *kpml4) :
    kobject {kobject::type::process},
    m_space {kpml4},
    m_next_handle {self_handle()+1},
    m_state {state::running}
{
}

process::~process()
{
    for (auto &it : m_handles)
        if (it.val) it.val->handle_release();
}

process & process::current() { return *current_cpu().process; }
process & process::kernel_process() { return g_kernel_process; }

process *
process::create_kernel_process(page_table *pml4)
{
    return new (&g_kernel_process) process {pml4};
}

void
process::exit(int32_t code)
{
    // TODO: make this thread-safe
    m_state = state::exited;
    m_return_code = code;
    close();

    for (auto *thread : m_threads) {
        thread->exit(code);
    }

    if (this == current_cpu().process)
        scheduler::get().schedule();
}

void
process::update()
{
    kassert(m_threads.count() > 0, "process::update with zero threads!");

    size_t i = 0;
    uint32_t status = 0;
    while (i < m_threads.count()) {
        thread *th = m_threads[i];
        if (th->has_state(thread::state::exited)) {
            status = th->m_return_code;
            m_threads.remove_swap_at(i);
            continue;
        }
        i++;
    }

    if (m_threads.count() == 0) {
        // TODO: What really is the return code in this case?
        exit(status);
    }
}

thread *
process::create_thread(uint8_t priority, bool user)
{
    if (priority == default_priority)
        priority = scheduler::default_priority;

    thread *th = new thread(*this, priority);
    kassert(th, "Failed to create thread!");

    if (user) {
        uintptr_t stack_top = stacks_top - (m_threads.count() * stack_size);

        vm_flags flags = vm_flags::zero|vm_flags::write;
        vm_area *vma = new vm_area_open(stack_size, flags);
        m_space.add(stack_top - stack_size, vma);

        // Space for null frame - because the page gets zeroed on
        // allocation, just pointing rsp here does the trick
        th->tcb()->rsp3 = stack_top - 2 * sizeof(uint64_t);
    }

    m_threads.append(th);
    scheduler::get().add_thread(th->tcb());
    return th;
}

bool
process::thread_exited(thread *th)
{
    kassert(&th->m_parent == this, "Process got thread_exited for non-child!");
    uint32_t status = th->m_return_code;
    m_threads.remove_swap(th);
    remove_handle(th->self_handle());
    delete th;

    // TODO: delete the thread's stack VMA

    if (m_threads.count() == 0) {
        exit(status);
        return true;
    }

    return false;
}

j6_handle_t
process::add_handle(kobject *obj)
{
    if (!obj)
        return j6_handle_invalid;

    obj->handle_retain();
    j6_handle_t handle = m_next_handle++;
    m_handles.insert(handle, obj);
    return handle;
}

bool
process::remove_handle(j6_handle_t handle)
{
    kobject *obj = m_handles.find(handle);
    if (obj) obj->handle_release();
    return m_handles.erase(handle);
}

kobject *
process::lookup_handle(j6_handle_t handle)
{
    return m_handles.find(handle);
}
