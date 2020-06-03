#include "apic.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_memory.h"
#include "log.h"
#include "msr.h"
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
	uintptr_t load_process_image(const void *image_start, size_t bytes, process *proc);
};

extern uint64_t idle_stack_end;

scheduler::scheduler(lapic *apic) :
	m_apic(apic),
	m_next_pid(1),
	m_clock(0)
{
	auto *idle = new process_node;
	uint8_t last_pri = num_priorities - 1;

	// The kernel idle task, also the thread we're in now
	idle->pid = 0;
	idle->ppid = 0;
	idle->priority = last_pri;
	idle->rsp = 0;  // This will get set when we switch away
	idle->rsp3 = 0; // Never used for the idle task
	idle->rsp0 = reinterpret_cast<uintptr_t>(&idle_stack_end);
	idle->pml4 = page_manager::get_pml4();
	idle->quanta = process_quanta;
	idle->flags =
		process_flags::running |
		process_flags::ready |
		process_flags::const_pri;

	m_runlists[last_pri].push_back(idle);
	m_current = idle;

	bsp_cpu_data.rsp0 = idle->rsp0;
	bsp_cpu_data.tcb = idle;
}

uintptr_t
load_process_image(const void *image_start, size_t bytes, process *proc)
{
	// We're now in the process space for this process, allocate memory for the
	// process code and load it
	page_manager *pager = page_manager::get();

	log::debug(logs::task, "Loading task! ELF: %016lx [%d]", image_start, bytes);

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

		log::debug(logs::task, "  Loadable segment %02u: vaddr %016lx  size %016lx",
			i, header->vaddr, header->mem_size);

		log::debug(logs::task, "         - aligned to: vaddr %016lx  pages %d",
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

		log::debug(logs::task, "  Loadable section %02u: vaddr %016lx  size %016lx",
			i, header->addr, header->size);

		void *dest = reinterpret_cast<void *>(header->addr);
		const void *src = kutil::offset_pointer(image_start, header->offset);
		kutil::memcpy(dest, src, header->size);
	}

	proc->flags &= ~process_flags::loading;

	uintptr_t entrypoint = image.entrypoint();
	log::debug(logs::task, "  Loaded! New process rip: %016lx", entrypoint);
	return entrypoint;
}

process_node *
scheduler::create_process(pid_t pid)
{
	kassert(pid <= 0, "Cannot specify a positive pid in create_process");

	auto *proc = new process_node;
	proc->pid = pid ? pid : m_next_pid++;
	proc->priority = default_priority;
	return proc;
}

void
scheduler::load_process(const char *name, const void *data, size_t size)
{
	auto *proc = create_process();

	uint16_t kcs = (1 << 3) | 0; // Kernel CS is GDT entry 1, ring 0
	uint16_t cs = (5 << 3) | 3;  // User CS is GDT entry 5, ring 3

	uint16_t kss = (2 << 3) | 0; // Kernel SS is GDT entry 2, ring 0
	uint16_t ss = (4 << 3) | 3;  // User SS is GDT entry 4, ring 3

	// Set up the page tables - this also allocates an initial user stack
	proc->pml4 = page_manager::get()->create_process_map();

	// Create an initial kernel stack space
	void *sp0 = proc->setup_kernel_stack();
	uintptr_t *stack = reinterpret_cast<uintptr_t *>(sp0) - 7;

	// Pass args to ramdisk_process_loader on the stack
	stack[0] = reinterpret_cast<uintptr_t>(data);
	stack[1] = reinterpret_cast<uintptr_t>(size);
	stack[2] = reinterpret_cast<uintptr_t>(proc);

	proc->rsp = reinterpret_cast<uintptr_t>(stack);
	proc->add_fake_task_return(
			reinterpret_cast<uintptr_t>(ramdisk_process_loader));

	// Arguments for iret - rip will be pushed on before these
	stack[3] = cs;
	stack[4] = rflags_int;
	stack[5] = initial_stack;
	stack[6] = ss;

	proc->rsp3 = initial_stack;
	proc->quanta = process_quanta;
	proc->flags =
		process_flags::running |
		process_flags::ready |
		process_flags::loading;

	m_runlists[default_priority].push_back(proc);

	log::debug(logs::task, "Creating process %s: pid %d  pri %d", name, proc->pid, proc->priority);
	log::debug(logs::task, "     RSP  %016lx", proc->rsp);
	log::debug(logs::task, "     RSP0 %016lx", proc->rsp0);
	log::debug(logs::task, "     PML4 %016lx", proc->pml4);
}

void
scheduler::create_kernel_task(pid_t pid, void (*task)())
{
	auto *proc = create_process(pid);

	uint16_t kcs = (1 << 3) | 0; // Kernel CS is GDT entry 1, ring 0
	uint16_t kss = (2 << 3) | 0; // Kernel SS is GDT entry 2, ring 0

	// Create an initial kernel stack space
	proc->setup_kernel_stack();
	proc->add_fake_task_return(
			reinterpret_cast<uintptr_t>(task));

	proc->pml4 = page_manager::get()->get_kernel_pml4();
	proc->quanta = process_quanta;
	proc->flags =
		process_flags::running |
		process_flags::ready;

	m_runlists[default_priority].push_back(proc);

	log::debug(logs::task, "Creating kernel task: pid %d  pri %d", proc->pid, proc->priority);
	log::debug(logs::task, "    RSP0 %016lx", proc->rsp0);
	log::debug(logs::task, "     RSP %016lx", proc->rsp);
	log::debug(logs::task, "    PML4 %016lx", proc->pml4);
}

void
scheduler::start()
{
	log::info(logs::task, "Starting scheduler.");
	wrmsr(msr::ia32_gs_base, reinterpret_cast<uintptr_t>(&bsp_cpu_data));
	m_tick_count = m_apic->enable_timer(isr::isrTimer, quantum_micros, false);
}

void scheduler::prune(uint64_t now)
{
	// Find processes that aren't ready or aren't running and
	// move them to the appropriate lists.
	for (auto &pri_list : m_runlists) {
		auto *proc = pri_list.front();
		while (proc) {
			bool running = proc->flags && process_flags::running;
			bool ready = proc->flags && process_flags::ready;
			if (running && ready) {
				proc = proc->next();
				continue;
			}

			auto *remove = proc;
			proc = proc->next();
			pri_list.remove(remove);

			if (!(remove->flags && process_flags::running)) {
				auto *parent = get_process_by_id(remove->ppid);
				if (parent && parent->wake_on_child(remove)) {
					m_blocked.remove(parent);
					m_runlists[parent->priority].push_front(parent);
					delete remove;
				} else {
					m_exited.push_back(remove);
				}
			} else {
				m_blocked.push_back(remove);
			}
		}
	}

	// Find blocked processes that are ready (possibly after waking wating
	// ones) and move them to the appropriate runlist.
	auto *proc = m_blocked.front();
	while (proc) {
		bool ready = proc->flags && process_flags::ready;
		ready |= proc->wake_on_time(now);

		auto *remove = proc;
		proc = proc->next();
		if (!ready) continue;

		m_blocked.remove(remove);
		m_runlists[remove->priority].push_front(remove);
	}
}

void
scheduler::schedule()
{
	pid_t lastpid = m_current->pid;

	m_runlists[m_current->priority].remove(m_current);
	if (m_current->flags && process_flags::ready) {
		m_runlists[m_current->priority].push_back(m_current);
	} else {
		m_blocked.push_back(m_current);
	}

	prune(++m_clock);

	uint8_t pri = 0;
	while (m_runlists[pri].empty()) {
		++pri;
		kassert(pri < num_priorities, "All runlists are empty");
	}

	m_current = m_runlists[pri].pop_front();

	if (lastpid != m_current->pid) {
		task_switch(m_current);

		bool loading = m_current->flags && process_flags::loading;
		log::debug(logs::task, "Scheduler switched to process %d, priority %d%s @ %lld.",
				m_current->pid, m_current->priority, loading ? " (loading)" : "", m_clock);
	}
}

void
scheduler::tick()
{
	if (--m_current->quanta == 0) {
		m_current->quanta = process_quanta;
		schedule();
	}
	m_apic->reset_timer(m_tick_count);
}

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
