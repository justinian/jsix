#include <stddef.h>

#include "apic.h"
#include "clock.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
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

#include "kutil/assert.h"


scheduler *scheduler::s_instance = nullptr;

extern uintptr_t fb_loc;
extern size_t fb_size;

const uint64_t rflags_noint = 0x002;
const uint64_t rflags_int = 0x202;

extern "C" {
	void preloaded_process_init();
	uintptr_t load_process_image(uintptr_t phys, uintptr_t virt, size_t bytes, TCB *tcb);
};

extern uint64_t idle_stack_end;

scheduler::scheduler(lapic *apic) :
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

	bsp_cpu_data.rsp0 = tcb->rsp0;
	bsp_cpu_data.tcb = tcb;
	bsp_cpu_data.p = kp;
	bsp_cpu_data.t = idle;
}

uintptr_t
load_process_image(uintptr_t phys, uintptr_t virt, size_t bytes, TCB *tcb)
{
	using memory::page_align_down;
	using memory::page_align_up;

	// We're now in the process space for this process, allocate memory for the
	// process code and load it
	process &proc = process::current();
	vm_space &space = proc.space();

	vm_area *vma = new vm_area_open(bytes, space, vm_flags::zero|vm_flags::write);
	space.add(virt, vma);
	vma->commit(phys, 0, memory::page_count(bytes));

	tcb->rsp3 -= 2 * sizeof(uint64_t);
	uint64_t *sentinel = reinterpret_cast<uint64_t*>(tcb->rsp3);
	sentinel[0] = sentinel[1] = 0;

	tcb->rsp3 -= sizeof(j6_process_init);
	j6_process_init *init = reinterpret_cast<j6_process_init*>(tcb->rsp3);

	init->process = proc.add_handle(&proc);
	init->handles[0] = proc.add_handle(system::get());
	init->handles[1] = j6_handle_invalid;
	init->handles[2] = j6_handle_invalid;

	// Crazypants framebuffer part
	init->handles[1] = reinterpret_cast<j6_handle_t>(fb_size);
	vma = new vm_area_open(fb_size, space, vm_flags::write|vm_flags::mmio);
	space.add(0x100000000, vma);
	vma->commit(fb_loc, 0, memory::page_count(fb_size));

	thread::current().clear_state(thread::state::loading);
	return tcb->rsp3;
}

thread *
scheduler::create_process(bool user)
{
	process *p = new process;
	thread *th = p->create_thread(default_priority, user);

	auto *tcb = th->tcb();
	tcb->time_left = quantum(default_priority);

	log::debug(logs::task, "Creating thread %llx, priority %d, time slice %d",
			th->koid(), tcb->priority, tcb->time_left);

	th->set_state(thread::state::ready);
	return th;
}

thread *
scheduler::load_process(uintptr_t phys, uintptr_t virt, size_t size, uintptr_t entry)
{

	uint16_t kcs = (1 << 3) | 0; // Kernel CS is GDT entry 1, ring 0
	uint16_t cs = (5 << 3) | 3;  // User CS is GDT entry 5, ring 3

	uint16_t kss = (2 << 3) | 0; // Kernel SS is GDT entry 2, ring 0
	uint16_t ss = (4 << 3) | 3;  // User SS is GDT entry 4, ring 3

	thread* th = create_process(true);
	auto *tcb = th->tcb();

	// Create an initial kernel stack space
	uintptr_t *stack = reinterpret_cast<uintptr_t *>(tcb->rsp0) - 9;

	// Pass args to preloaded_process_init on the stack
	stack[0] = reinterpret_cast<uintptr_t>(phys);
	stack[1] = reinterpret_cast<uintptr_t>(virt);
	stack[2] = reinterpret_cast<uintptr_t>(size);
	stack[3] = reinterpret_cast<uintptr_t>(tcb);

	tcb->rsp = reinterpret_cast<uintptr_t>(stack);
	th->add_thunk_kernel(reinterpret_cast<uintptr_t>(preloaded_process_init));

	// Arguments for iret - rip will be pushed on before these
	stack[4] = reinterpret_cast<uintptr_t>(entry);
	stack[5] = cs;
	stack[6] = rflags_int | (3 << 12);
	stack[7] = process::stacks_top;
	stack[8] = ss;

	tcb->rsp3 = process::stacks_top;

	log::debug(logs::task, "Loading thread %llx  pri %d", th->koid(), tcb->priority);
	log::debug(logs::task, "     RSP  %016lx", tcb->rsp);
	log::debug(logs::task, "     RSP0 %016lx", tcb->rsp0);
	log::debug(logs::task, "     PML4 %016lx", tcb->pml4);

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
	log::info(logs::task, "Starting scheduler.");
	wrmsr(msr::ia32_gs_base, reinterpret_cast<uintptr_t>(&bsp_cpu_data));
	m_apic->enable_timer(isr::isrTimer, false);
	m_apic->reset_timer(10);
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
			log::debug(logs::task, "Prune: readying unblocked thread %llx", th->koid());
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
				log::debug(logs::task, "Scheduler promoting thread %llx, priority %d",
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
	uint32_t remaining = m_apic->stop_timer();
	m_current->time_left = remaining;
	thread *th = thread::from_tcb(m_current);
	const bool constant = th->has_state(thread::state::constant);

	if (remaining == 0) {
		if (priority < max_priority && !constant) {
			// Process used its whole timeslice, demote it
			++m_current->priority;
			log::debug(logs::task, "Scheduler  demoting thread %llx, priority %d",
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
	m_apic->reset_timer(next->time_left);

	if (next != m_current) {
		thread *next_thread = thread::from_tcb(next);

		bsp_cpu_data.t = next_thread;
		bsp_cpu_data.p = &next_thread->parent();
		m_current = next;

		log::debug(logs::task, "Scheduler switching threads %llx->%llx",
				th->koid(), next_thread->koid());
		log::debug(logs::task, "    priority %d time left %d @ %lld.",
				m_current->priority, m_current->time_left, m_clock);
		log::debug(logs::task, "    PML4 %llx", m_current->pml4);

		task_switch(m_current);
	}
}
