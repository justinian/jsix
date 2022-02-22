#include <stddef.h>

#include <util/spinlock.h>

#include "apic.h"
#include "assert.h"
#include "clock.h"
#include "cpu.h"
#include "debug.h"
#include "device_manager.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "logger.h"
#include "msr.h"
#include "objects/channel.h"
#include "objects/process.h"
#include "objects/system.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "scheduler.h"

using obj::process;
using obj::thread;

extern "C" void task_switch(TCB *tcb);
scheduler *scheduler::s_instance = nullptr;

struct run_queue
{
    tcb_node *current = nullptr;
    tcb_list ready[scheduler::num_priorities];
    tcb_list blocked;

    uint64_t last_promotion = 0;
    uint64_t last_steal = 0;
    util::spinlock lock;
};

scheduler::scheduler(unsigned cpus) :
    m_next_pid {1}
{
    kassert(!s_instance, "Created multiple schedulers!");
    if (!s_instance)
        s_instance = this;

    m_run_queues.set_size(cpus);
}

scheduler::~scheduler()
{
    // Not truly necessary - if the scheduler is going away, the whole
    // system is probably going down. But let's be clean.
    if (s_instance == this)
        s_instance = nullptr;
}

template <typename T>
inline T * push(uintptr_t &rsp, size_t size = sizeof(T)) {
    rsp -= size;
    T *p = reinterpret_cast<T*>(rsp);
    rsp &= ~(sizeof(uint64_t)-1); // Align the stack
    return p;
}

void
scheduler::create_kernel_task(void (*task)(), uint8_t priority, bool constant)
{
    thread *th = process::kernel_process().create_thread(0, priority);
    auto *tcb = th->tcb();

    th->add_thunk_kernel(reinterpret_cast<uintptr_t>(task));

    tcb->time_left = quantum(priority);
    if (constant)
        th->set_state(thread::state::constant);

    th->set_state(thread::state::ready);

    log::debug(logs::task, "Creating kernel task: thread %llx  pri %d", th->koid(), tcb->priority);
    log::debug(logs::task, "    RSP0 %016lx", tcb->rsp0);
    log::debug(logs::task, "     RSP %016lx", tcb->rsp);
    log::debug(logs::task, "    PML4 %016lx", tcb->pml4);
}

uint32_t
scheduler::quantum(int priority)
{
    return quantum_micros << priority;
}

void
scheduler::start()
{
    cpu_data &cpu = current_cpu();
    run_queue &queue = m_run_queues[cpu.index];

    {
        util::scoped_lock lock {queue.lock};
        thread *idle = cpu.thread;
        queue.current = idle->tcb();
    }

    cpu.apic->enable_timer(isr::isrTimer, false);
    cpu.apic->reset_timer(10);
}

void
scheduler::add_thread(TCB *t)
{
    cpu_data &cpu = current_cpu();
    run_queue &queue = m_run_queues[cpu.index];
    util::scoped_lock lock {queue.lock};

    queue.blocked.push_back(static_cast<tcb_node*>(t));
    t->time_left = quantum(t->priority);
}

void
scheduler::prune(run_queue &queue, uint64_t now)
{
    // Find processes that are ready or have exited and
    // move them to the appropriate lists.
    auto *tcb = queue.blocked.front();
    while (tcb) {
        thread *th = tcb->thread;
        uint8_t priority = tcb->priority;

        uint64_t timeout = th->wake_timeout();
        if (timeout && timeout <= now)
            th->wake();

        bool ready = th->has_state(thread::state::ready);
        bool exited = th->has_state(thread::state::exited);
        bool constant = th->has_state(thread::state::constant);
        bool current = tcb == queue.current;

        auto *remove = tcb;
        tcb = tcb->next();
        if (!exited && !ready)
            continue;

        if (exited) {
            // If the current thread has exited, wait until the next call
            // to prune() to delete it, because we may be deleting our current
            // page tables
            if (current) continue;

            queue.blocked.remove(remove);
            process &p = th->parent();

            // thread_exited deletes the thread, and returns true if the process
            // should also now be deleted
            if(!current && p.thread_exited(th))
                delete &p;
        } else {
            queue.blocked.remove(remove);
            log::debug(logs::sched, "Prune: readying unblocked thread %llx", th->koid());
            queue.ready[remove->priority].push_back(remove);
        }
    }
}

void
scheduler::check_promotions(run_queue &queue, uint64_t now)
{
    for (auto &pri_list : queue.ready) {
        for (auto *tcb : pri_list) {
            const thread *th = tcb->thread;
            const bool constant = th->has_state(thread::state::constant);
            if (constant)
                continue;

            const uint64_t age = now - tcb->last_ran;
            const uint8_t priority = tcb->priority;

            bool stale =
                age > quantum(priority) * 2 &&
                tcb->priority > promote_limit &&
                !constant;

            if (stale) {
                // If the thread is stale, promote it
                queue.ready[priority].remove(tcb);
                tcb->priority -= 1;
                tcb->time_left = quantum(tcb->priority);
                queue.ready[tcb->priority].push_back(tcb);
                log::info(logs::sched, "Scheduler promoting thread %llx, priority %d",
                        th->koid(), tcb->priority);
            }
        }
    }

    queue.last_promotion = now;
}

static size_t
balance_lists(tcb_list &to, tcb_list &from)
{
    size_t to_len = to.length();
    size_t from_len = from.length();

    // Only steal from the rich, don't be Dennis Moore
    if (from_len <= to_len)
        return 0;

    size_t steal = (from_len - to_len) / 2;
    for (size_t i = 0; i < steal; ++i)
        to.push_front(from.pop_front());
    return steal;
}

void
scheduler::steal_work(cpu_data &cpu)
{
    // Lock this cpu's queue for the whole time while we modify it
    run_queue &my_queue = m_run_queues[cpu.index];
    util::scoped_lock my_queue_lock {my_queue.lock};

    const unsigned count = m_run_queues.count();
    for (unsigned i = 0; i < count; ++i) {
        if (i == cpu.index) continue;

        run_queue &other_queue = m_run_queues[i];
        util::scoped_lock other_queue_lock {other_queue.lock};

        size_t stolen = 0;

        // Don't steal from max_priority, that's the idle thread
        for (unsigned pri = 0; pri < max_priority; ++pri)
            stolen += balance_lists(my_queue.ready[pri], other_queue.ready[pri]);

        stolen += balance_lists(my_queue.blocked, other_queue.blocked);

        if (stolen)
            log::debug(logs::sched, "CPU%02x stole %2d tasks from CPU%02x",
                    cpu.index, stolen, i);
    }
}

void
scheduler::schedule()
{
    cpu_data &cpu = current_cpu();
    run_queue &queue = m_run_queues[cpu.index];
    lapic &apic = *cpu.apic;
    uint32_t remaining = apic.stop_timer();

    uint64_t now = clock::get().value();

    // Only one CPU can be stealing at a time
    if (m_steal_turn == cpu.index &&
        now - queue.last_steal > steal_frequency) {
        steal_work(cpu);
        queue.last_steal = now;
        m_steal_turn = (m_steal_turn + 1) % m_run_queues.count();
    }

    // We need to explicitly lock/unlock here instead of
    // using a scoped lock, because the scope doesn't "end"
    // for the current thread until it gets scheduled again
    util::spinlock::waiter waiter;
    queue.lock.acquire(&waiter);

    queue.current->time_left = remaining;
    thread *th = queue.current->thread;
    uint8_t priority = queue.current->priority;
    const bool constant = th->has_state(thread::state::constant);

    if (remaining == 0) {
        if (priority < max_priority && !constant) {
            // Process used its whole timeslice, demote it
            ++queue.current->priority;
            log::debug(logs::sched, "Scheduler  demoting thread %llx, priority %d",
                    th->koid(), queue.current->priority);
        }
        queue.current->time_left = quantum(queue.current->priority);
    } else if (remaining > 0) {
        // Process gave up CPU, give it a small bonus to its
        // remaining timeslice.
        uint32_t bonus = quantum(priority) >> 4;
        queue.current->time_left += bonus;
    }

    if (th->has_state(thread::state::ready)) {
        queue.ready[queue.current->priority].push_back(queue.current);
    } else {
        queue.blocked.push_back(queue.current);
    }

    clock::get().update();
    prune(queue, now);
    if (now - queue.last_promotion > promote_frequency)
        check_promotions(queue, now);

    priority = 0;
    while (queue.ready[priority].empty()) {
        ++priority;
        kassert(priority < num_priorities, "All runlists are empty");
    }

    queue.current->last_ran = now;

    auto *next = queue.ready[priority].pop_front();
    next->last_ran = now;
    apic.reset_timer(next->time_left);

    if (next == queue.current) {
        queue.lock.release(&waiter);
        return;
    }

    thread *next_thread = next->thread;

    cpu.thread = next_thread;
    cpu.process = &next_thread->parent();
    queue.current = next;

    log::debug(logs::sched, "CPU%02x switching threads %llx->%llx",
            cpu.index, th->koid(), next_thread->koid());
    log::debug(logs::sched, "    priority %d time left %d @ %lld.",
            next->priority, next->time_left, now);
    log::debug(logs::sched, "    PML4 %llx", next->pml4);

    queue.lock.release(&waiter);
    task_switch(queue.current);
}
