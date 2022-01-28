#include <util/no_construct.h>

#include "assert.h"
#include "cpu.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "scheduler.h"


// This object is initialized _before_ global constructors are called,
// so we don't want it to have a global constructor at all, lest it
// overwrite the previous initialization.
static util::no_construct<obj::process> __g_kernel_process_storage;
obj::process &g_kernel_process = __g_kernel_process_storage.value;


namespace obj {

process::process() :
    kobject {kobject::type::process},
    m_next_handle {1},
    m_state {state::running}
{
    m_self_handle = add_handle(this, process::self_caps);
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
process::create_thread(uintptr_t rsp3, uint8_t priority)
{
    if (priority == default_priority)
        priority = scheduler::default_priority;

    thread *th = new thread(*this, priority);
    kassert(th, "Failed to create thread!");

    if (rsp3)
        th->tcb()->rsp3 = rsp3;

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
process::add_handle(kobject *obj, j6_cap_t caps)
{
    if (!obj)
        return j6_handle_invalid;

    handle h {m_next_handle++, obj, caps};
    j6_handle_t id = h.id;

    m_handles.insert(id, h);
    return id;
}

bool
process::remove_handle(j6_handle_t id)
{
    return m_handles.erase(id);
}

handle *
process::lookup_handle(j6_handle_t id)
{
    return m_handles.find(id);
}

size_t
process::list_handles(j6_handle_t *handles, size_t len)
{
    for (const auto &i : m_handles) {
        if (len-- == 0)
            break;
        *handles++ = i.key;
    }

    return m_handles.count();
}

} // namespace obj
