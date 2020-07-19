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
#include "objects/process.h"
#include "page_manager.h"
#include "scheduler.h"

#include "elf/elf.h"
#include "kutil/assert.h"

using memory::initial_stack;

scheduler scheduler::s_instance(nullptr);

const uint64_t rflags_noint = 0x002;
const uint64_t rflags_int = 0x202;

extern "C" {
	void ramdisk_process_loader();
	uintptr_t load_process_image(const void *image_start, size_t bytes, TCB *tcb);
};

extern uint64_t idle_stack_end;

/// Set up a new empty kernel stack for this thread. Sets rsp0 on the
/// TCB object, but also returns it.
/// \returns   The new rsp0 as a pointer
static void *
setup_kernel_stack(TCB *tcb)
{
	constexpr size_t initial_stack_size = 0x1000;
	constexpr unsigned null_frame_entries = 2;
	constexpr size_t null_frame_size = null_frame_entries * sizeof(uint64_t);

	void *stack_bottom = kutil::kalloc(initial_stack_size);
	kutil::memset(stack_bottom, 0, initial_stack_size);

	log::debug(logs::memory, "Created kernel stack at %016lx size 0x%lx",
			stack_bottom, initial_stack_size);

	void *stack_top =
		kutil::offset_pointer(stack_bottom,
				initial_stack_size - null_frame_size);

	uint64_t *null_frame = reinterpret_cast<uint64_t*>(stack_top);
	for (unsigned i = 0; i < null_frame_entries; ++i)
		null_frame[i] = 0;

	tcb->kernel_stack_size = initial_stack_size;
	tcb->kernel_stack = reinterpret_cast<uintptr_t>(stack_bottom);
	tcb->rsp0 = reinterpret_cast<uintptr_t>(stack_top);
	tcb->rsp = tcb->rsp0;

	return stack_top;
}

/// Initialize this process' kenrel stack with a fake return segment for
/// returning out of task_switch.
/// \arg tcb  TCB of the thread to modify
/// \arg rip  The rip to return to
static void
add_fake_task_return(TCB *tcb, uintptr_t rip)
{
	tcb->rsp -= sizeof(uintptr_t) * 7;
	uintptr_t *stack = reinterpret_cast<uintptr_t*>(tcb->rsp);

	stack[6] = rip;        // return rip
	stack[5] = tcb->rsp0;  // rbp
	stack[4] = 0xbbbbbbbb; // rbx
	stack[3] = 0x12121212; // r12
	stack[2] = 0x13131313; // r13
	stack[1] = 0x14141414; // r14
	stack[0] = 0x15151515; // r15
}

scheduler::scheduler(lapic *apic) :
	m_apic(apic),
	m_next_pid(1),
	m_clock(0),
	m_last_promotion(0)
{
	page_table *pml4 = page_manager::get_pml4();
	m_kernel_process = new process(pml4);
	thread *idle = m_kernel_process->create_thread(max_priority);

	auto *tcb = idle->tcb();

	log::debug(logs::task, "Idle thread koid %llx", idle->koid());

	// The kernel idle task, also the thread we're in now
	tcb->rsp = 0;  // This will get set when we switch away
	tcb->rsp3 = 0; // Never used for the idle task
	tcb->rsp0 = reinterpret_cast<uintptr_t>(&idle_stack_end);

	idle->set_state(thread::state::constant);

	m_runlists[max_priority].push_back(tcb);
	m_current = tcb;

	bsp_cpu_data.rsp0 = tcb->rsp0;
	bsp_cpu_data.tcb = tcb;
}

uintptr_t
load_process_image(const void *image_start, size_t bytes, TCB *tcb)
{
	// We're now in the process space for this process, allocate memory for the
	// process code and load it
	page_manager *pager = page_manager::get();

	thread *th = tcb->thread_data;

	log::debug(logs::loader, "Loading task! ELF: %016lx [%d]", image_start, bytes);

	// TODO: Handle bad images gracefully
	elf::elf image(image_start, bytes);
	kassert(image.valid(), "Invalid ELF passed to load_process_image");

	const unsigned program_count = image.program_count();
	for (unsigned i = 0; i < program_count; ++i) {
		const elf::program_header *header = image.program(i);

		if (header->type != elf::segment_type::load)
			continue;

		uintptr_t aligned = header->vaddr & ~(memory::frame_size - 1);
		size_t size = (header->vaddr + header->mem_size) - aligned;
		size_t pages = page_manager::page_count(size);

		log::debug(logs::loader, "  Loadable segment %02u: vaddr %016lx  size %016lx",
			i, header->vaddr, header->mem_size);

		log::debug(logs::loader, "         - aligned to: vaddr %016lx  pages %d",
			aligned, pages);

		void *mapped = pager->map_pages(aligned, pages, true);
		kassert(mapped, "Tried to map userspace pages and failed!");

		kutil::memset(mapped, 0, pages * memory::frame_size);
	}

	const unsigned section_count = image.section_count();
	for (unsigned i = 0; i < section_count; ++i) {
		const elf::section_header *header = image.section(i);

		if (header->type != elf::section_type::progbits ||
			!bitfield_has(header->flags, elf::section_flags::alloc))
			continue;

		log::debug(logs::loader, "  Loadable section %02u: vaddr %016lx  size %016lx",
			i, header->addr, header->size);

		void *dest = reinterpret_cast<void *>(header->addr);
		const void *src = kutil::offset_pointer(image_start, header->offset);
		kutil::memcpy(dest, src, header->size);
	}

	th->clear_state(thread::state::loading);

	uintptr_t entrypoint = image.entrypoint();
	log::debug(logs::loader, "  Loaded! New thread rip: %016lx", entrypoint);
	return entrypoint;
}

thread *
scheduler::create_process(page_table *pml4)
{
	process *p = new process(pml4);
	thread *th = p->create_thread(default_priority);
	auto *tcb = th->tcb();

	tcb->time_left = quantum(default_priority) + startup_bonus;

	log::debug(logs::task, "Creating thread %llx, priority %d, time slice %d",
			th->koid(), tcb->priority, tcb->time_left);

	return th;
}

void
scheduler::load_process(const char *name, const void *data, size_t size)
{

	uint16_t kcs = (1 << 3) | 0; // Kernel CS is GDT entry 1, ring 0
	uint16_t cs = (5 << 3) | 3;  // User CS is GDT entry 5, ring 3

	uint16_t kss = (2 << 3) | 0; // Kernel SS is GDT entry 2, ring 0
	uint16_t ss = (4 << 3) | 3;  // User SS is GDT entry 4, ring 3

	// Set up the page tables - this also allocates an initial user stack
	page_table *pml4 = page_manager::get()->create_process_map();

	thread* th = create_process(pml4);
	auto *tcb = th->tcb();

	// Create an initial kernel stack space
	void *sp0 = setup_kernel_stack(tcb);
	uintptr_t *stack = reinterpret_cast<uintptr_t *>(sp0) - 7;

	// Pass args to ramdisk_process_loader on the stack
	stack[0] = reinterpret_cast<uintptr_t>(data);
	stack[1] = reinterpret_cast<uintptr_t>(size);
	stack[2] = reinterpret_cast<uintptr_t>(tcb);

	tcb->rsp = reinterpret_cast<uintptr_t>(stack);
	add_fake_task_return(tcb,
			reinterpret_cast<uintptr_t>(ramdisk_process_loader));

	// Arguments for iret - rip will be pushed on before these
	stack[3] = cs;
	stack[4] = rflags_int;
	stack[5] = initial_stack;
	stack[6] = ss;

	tcb->rsp3 = initial_stack;

	m_runlists[default_priority].push_back(tcb);

	log::debug(logs::task, "Loading thread %s: koid %llx  pri %d", name, th->koid(), tcb->priority);
	log::debug(logs::task, "     RSP  %016lx", tcb->rsp);
	log::debug(logs::task, "     RSP0 %016lx", tcb->rsp0);
	log::debug(logs::task, "     PML4 %016lx", tcb->pml4);
}

void
scheduler::create_kernel_task(void (*task)(), uint8_t priority, bool constant)
{
	page_table *pml4 = page_manager::get()->get_kernel_pml4();
	thread *th = create_process(pml4);
	auto *tcb = th->tcb();

	uint16_t kcs = (1 << 3) | 0; // Kernel CS is GDT entry 1, ring 0
	uint16_t kss = (2 << 3) | 0; // Kernel SS is GDT entry 2, ring 0

	// Create an initial kernel stack space
	setup_kernel_stack(tcb);
	add_fake_task_return(tcb,
			reinterpret_cast<uintptr_t>(task));

	tcb->priority = priority;
	tcb->pml4 = page_manager::get()->get_kernel_pml4();
	if (constant)
		th->set_state(thread::state::constant);

	m_runlists[default_priority].push_back(tcb);

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
		thread *th = tcb->thread_data;
		uint8_t priority = tcb->priority;

		bool ready = th->has_state(thread::state::ready);
		bool exited = th->has_state(thread::state::exited);
		bool constant = th->has_state(thread::state::constant);

		ready |= th->wake_on_time(now);

		auto *remove = tcb;
		tcb = tcb->next();
		if (!exited && !ready)
			continue;

		m_blocked.remove(remove);

		if (exited) {
			process &p = th->parent();
			if(p.thread_exited(th))
				delete &p;
		} else {
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
			const thread *th = m_current->thread_data;
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
	thread *th = m_current->thread_data;
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
		m_current = next;
		task_switch(m_current);

		log::debug(logs::task, "Scheduler switched to thread %llx, priority %d time left %d @ %lld.",
				th->koid(), m_current->priority, m_current->time_left, m_clock);
	}
}

/*
process_node *
scheduler::get_process_by_id(uint32_t pid)
{
	// TODO: this needs to be a hash map
	for (auto *proc : m_blocked) {
		if (proc->pid == pid) return proc;
	}

	for (int i = 0; i < num_priorities; ++i) {
		for (auto *proc : m_runlists[i]) {
			if (proc->pid == pid) return proc;
		}
	}

	for (auto *proc : m_exited) {
		if (proc->pid == pid) return proc;
	}

	return nullptr;
}
*/
