#include <stddef.h>

#include <j6/init.h>

#include "apic.h"
#include "clock.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "device_manager.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_memory.h"
#include "log.h"
#include "msr.h"
#include "objects/channel.h"
#include "objects/process.h"
#include "objects/system.h"
#include "objects/vm_area.h"
#include "scheduler.h"

// here for the framebuffer hack
#include "kernel_args.h"

#include "kutil/assert.h"


scheduler *scheduler::s_instance = nullptr;

const uint64_t rflags_noint = 0x002;
const uint64_t rflags_int = 0x202;

extern uint64_t idle_stack_end;

extern "C" void task_switch(TCB *tcb);

scheduler::scheduler(lapic &apic) :
	m_apic(apic),
	m_next_pid(1),
	m_clock(0),
	m_last_promotion(0)
{
	kassert(!s_instance, "Multiple schedulers created!");
	s_instance = this;

	process *kp = &process::kernel_process();

	log::debug(logs::task, "Kernel process koid %llx", kp->koid());

	thread *idle = thread::create_idle_thread(*kp, max_priority,
		reinterpret_cast<uintptr_t>(&idle_stack_end));

	log::debug(logs::task, "Idle thread koid %llx", idle->koid());

	auto *tcb = idle->tcb();
	m_runlists[max_priority].push_back(tcb);
	m_current = tcb;

	cpu_data &cpu = current_cpu();
	cpu.rsp0 = tcb->rsp0;
	cpu.tcb = tcb;
	cpu.process = kp;
	cpu.thread = idle;
}

template <typename T>
inline T * push(uintptr_t &rsp, size_t size = sizeof(T)) {
	rsp -= size;
	T *p = reinterpret_cast<T*>(rsp);
	rsp &= ~(sizeof(uint64_t)-1); // Align the stack
	return p;
}

thread *
scheduler::create_process(bool user)
{
	process *p = new process;
	thread *th = p->create_thread(default_priority, user);

	TCB *tcb = th->tcb();
	log::debug(logs::task, "Creating thread %llx, priority %d, time slice %d",
			th->koid(), tcb->priority, tcb->time_left);

	th->set_state(thread::state::ready);
	return th;
}

void
scheduler::create_kernel_task(void (*task)(), uint8_t priority, bool constant)
{
	thread *th = process::kernel_process().create_thread(priority, false);
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
	log::info(logs::sched, "Starting scheduler.");
	m_apic.enable_timer(isr::isrTimer, false);
	m_apic.reset_timer(10);
}

void
scheduler::add_thread(TCB *t)
{
	m_blocked.push_back(static_cast<tcb_node*>(t));
	t->time_left = quantum(t->priority);

}

void scheduler::prune(uint64_t now)
{
	// Find processes that are ready or have exited and
	// move them to the appropriate lists.
	auto *tcb = m_blocked.front();
	while (tcb) {
		thread *th = thread::from_tcb(tcb);
		uint8_t priority = tcb->priority;

		bool ready = th->has_state(thread::state::ready);
		bool exited = th->has_state(thread::state::exited);
		bool constant = th->has_state(thread::state::constant);
		bool current = tcb == m_current;

		ready |= th->wake_on_time(now);

		auto *remove = tcb;
		tcb = tcb->next();
		if (!exited && !ready)
			continue;

		if (exited) {
			// If the current thread has exited, wait until the next call
			// to prune() to delete it, because we may be deleting our current
			// page tables
			if (current) continue;

			m_blocked.remove(remove);
			process &p = th->parent();

			// thread_exited deletes the thread, and returns true if the process
			// should also now be deleted
			if(!current && p.thread_exited(th))
				delete &p;
		} else {
			m_blocked.remove(remove);
			log::debug(logs::sched, "Prune: readying unblocked thread %llx", th->koid());
			m_runlists[remove->priority].push_back(remove);
		}
	}
}

void
scheduler::check_promotions(uint64_t now)
{
	for (auto &pri_list : m_runlists) {
		for (auto *tcb : pri_list) {
			const thread *th = thread::from_tcb(m_current);
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
				m_runlists[priority].remove(tcb);
				tcb->priority -= 1;
				tcb->time_left = quantum(tcb->priority);
				m_runlists[tcb->priority].push_back(tcb);
				log::info(logs::sched, "Scheduler promoting thread %llx, priority %d",
						th->koid(), tcb->priority);
			}
		}
	}

	m_last_promotion = now;
}

void
scheduler::schedule()
{
	uint8_t priority = m_current->priority;
	uint32_t remaining = m_apic.stop_timer();
	m_current->time_left = remaining;
	thread *th = thread::from_tcb(m_current);
	const bool constant = th->has_state(thread::state::constant);

	if (remaining == 0) {
		if (priority < max_priority && !constant) {
			// Process used its whole timeslice, demote it
			++m_current->priority;
			log::debug(logs::sched, "Scheduler  demoting thread %llx, priority %d",
					th->koid(), m_current->priority);
		}
		m_current->time_left = quantum(m_current->priority);
	} else if (remaining > 0) {
		// Process gave up CPU, give it a small bonus to its
		// remaining timeslice.
		uint32_t bonus = quantum(priority) >> 4;
		m_current->time_left += bonus;
	}

	m_runlists[priority].remove(m_current);
	if (th->has_state(thread::state::ready)) {
		m_runlists[m_current->priority].push_back(m_current);
	} else {
		m_blocked.push_back(m_current);
	}

	clock::get().update();
	prune(++m_clock);
	if (m_clock - m_last_promotion > promote_frequency)
		check_promotions(m_clock);

	priority = 0;
	while (m_runlists[priority].empty()) {
		++priority;
		kassert(priority < num_priorities, "All runlists are empty");
	}

	m_current->last_ran = m_clock;

	auto *next = m_runlists[priority].pop_front();
	next->last_ran = m_clock;
	m_apic.reset_timer(next->time_left);

	if (next != m_current) {
		thread *next_thread = thread::from_tcb(next);

		cpu_data &cpu = current_cpu();
		cpu.thread = next_thread;
		cpu.process = &next_thread->parent();
		m_current = next;

		log::debug(logs::sched, "Scheduler switching threads %llx->%llx",
				th->koid(), next_thread->koid());
		log::debug(logs::sched, "    priority %d time left %d @ %lld.",
				m_current->priority, m_current->time_left, m_clock);
		log::debug(logs::sched, "    PML4 %llx", m_current->pml4);

		task_switch(m_current);
	}
}
