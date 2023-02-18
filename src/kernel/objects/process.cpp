#include <new>

#include <util/no_construct.h>

#include "assert.h"
#include "capabilities.h"
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
    m_state {state::running}
{
    m_self_handle = g_cap_table.create(this, process::self_caps);
    add_handle(m_self_handle);
}

// The "kernel process"-only constructor
process::process(page_table *kpml4) :
    kobject {kobject::type::process},
    m_space {kpml4},
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
    if (m_state == state::exited)
        return;

    m_state = state::exited;
    m_return_code = code;

    thread &current = thread::current();

    util::scoped_lock lock {m_threads_lock};
    for (auto *thread : m_threads) {
        if (thread != &current)
            thread->exit();
    }

    lock.release();
    if (&current.parent() == this)
        current.exit();
}

void
process::update()
{
    util::scoped_lock lock {m_threads_lock};

    kassert(false, "process::update used!");
    kassert(m_threads.count() > 0, "process::update with zero threads!");

    size_t i = 0;
    while (i < m_threads.count()) {
        thread *th = m_threads[i];
        if (th->has_state(thread::state::exited)) {
            m_threads.remove_swap_at(i);
            continue;
        }
        i++;
    }

    if (m_threads.count() == 0)
        exit(-1);
}

thread *
process::create_thread(uintptr_t rsp3, uint8_t priority)
{
    util::scoped_lock lock {m_threads_lock};

    if (m_state == state::exited)
        return nullptr;

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
    util::scoped_lock lock {m_threads_lock};

    kassert(&th->m_parent == this, "Process got thread_exited for non-child!");
    m_threads.remove_swap(th);
    remove_handle(th->self_handle());
    delete th;

    // TODO: delete the thread's stack VMA
    if (m_threads.empty() && m_state != state::exited) {
        exit(-1);
        return true;
    }

    return false;
}

void
process::add_handle(j6_handle_t handle)
{
    capability *c = g_cap_table.retain(handle);
    kassert(c, "Trying to add a non-existant handle to a process!");

    if (c) {
        util::scoped_lock lock {m_handles_lock};
        m_handles.add(handle);
    }
}

bool
process::remove_handle(j6_handle_t handle)
{
    util::scoped_lock lock {m_handles_lock};
    bool removed = m_handles.remove(handle);
    lock.release();

    if (removed)
        g_cap_table.release(handle);

    return removed;
}

bool
process::has_handle(j6_handle_t handle)
{
    util::scoped_lock lock {m_handles_lock};
    return m_handles.contains(handle);
}

size_t
process::list_handles(j6_handle_descriptor *handles, size_t len)
{
    util::scoped_lock lock {m_handles_lock};

    size_t count = 0;
    for (j6_handle_t handle : m_handles) {

        capability *cap = g_cap_table.find_without_retain(handle);
        kassert(cap, "Found process handle that wasn't in the cap table");
        if (!cap) continue;

        j6_handle_descriptor &desc = handles[count];
        desc.handle = handle;
        desc.caps = cap->caps;
        desc.type = static_cast<j6_object_type>(cap->type);

        if (++count == len) break;
    }

    return m_handles.count();
}

} // namespace obj
