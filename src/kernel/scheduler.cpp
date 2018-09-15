#include "apic.h"
#include "console.h"
#include "cpu.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"
#include "msr.h"
#include "page_manager.h"
#include "scheduler.h"

#include "elf/elf.h"
#include "kutil/assert.h"

scheduler scheduler::s_instance(nullptr);
static const uint32_t quantum = 2000000;
//static const uint32_t quantum =  20000000;

const int stack_size = 0x1000;
const uint64_t rflags_noint = 0x002;
const uint64_t rflags_int = 0x202;

extern "C" {
	void ramdisk_process_loader();
	void load_process(const void *image_start, size_t bytes, process *proc, cpu_state state);
};

scheduler::scheduler(lapic *apic) :
	m_apic(apic),
	m_next_pid(1)
{
	auto *idle = m_process_allocator.pop();

	uint8_t last_pri = num_priorities - 1;

	// The kernel idle task, also the thread we're in now
	idle->pid = 0;
	idle->ppid = 0;
	idle->priority = last_pri;
	idle->rsp = 0; // This will get set when we switch away
	idle->pml4 = page_manager::get_pml4();
	idle->flags =
		process_flags::ready |
		process_flags::const_pri;

	m_runlists[last_pri].push_back(idle);
	m_current = idle;
}

void
load_process(const void *image_start, size_t bytes, process *proc, cpu_state state)
{
	// We're now in the process space for this process, allocate memory for the
	// process code and load it
	page_manager *pager = page_manager::get();

	log::debug(logs::task, "Loading task! ELF: %016lx [%d]", image_start, bytes);

	// TODO: Handle bad images gracefully
	elf::elf image(image_start, bytes);
	kassert(image.valid(), "Invalid ELF passed to load_process");

	const unsigned program_count = image.program_count();
	for (unsigned i = 0; i < program_count; ++i) {
		const elf::program_header *header = image.program(i);

		if (header->type != elf::segment_type::load)
			continue;

		addr_t aligned = header->vaddr & ~(page_manager::page_size - 1);
		size_t size = (header->vaddr + header->mem_size) - aligned;
		size_t pages = page_manager::page_count(size);

		log::debug(logs::task, "  Loadable segment %02u: vaddr %016lx  size %016lx",
			i, header->vaddr, header->mem_size);

		log::debug(logs::task, "         - aligned to: vaddr %016lx  pages %d",
			aligned, pages);

		void *mapped = pager->map_pages(aligned, pages, true);
		kassert(mapped, "Tried to map userspace pages and failed!");

		kutil::memset(mapped, 0, pages * page_manager::page_size);
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

	state.rip = image.entrypoint();
	proc->flags &= ~process_flags::loading;

	log::debug(logs::task, "  Loaded! New process rip: %016lx", state.rip);
}

void
scheduler::create_process(const char *name, const void *data, size_t size)
{
	uint16_t kcs = (1 << 3) | 0; // Kernel CS is GDT entry 1, ring 0
	uint16_t cs = (5 << 3) | 3;  // User CS is GDT entry 5, ring 3

	uint16_t kss = (2 << 3) | 0; // Kernel SS is GDT entry 2, ring 0
	uint16_t ss = (4 << 3) | 3;  // User SS is GDT entry 4, ring 3

	// Set up the page tables - this also allocates an initial user stack
	page_table *pml4 = page_manager::get()->create_process_map();

	// Create a one-page kernel stack space
	void *stack0 = kutil::malloc(stack_size);
	kutil::memset(stack0, 0, stack_size);

	// Stack grows down, point to the end
	void *sp0 = kutil::offset_pointer(stack0, stack_size);

	cpu_state *state = reinterpret_cast<cpu_state *>(sp0) - 1;

	// Highest state in the stack is the process' kernel stack for the loader
	// to iret to:
	state->ds = state->ss = ss;
	state->cs = cs;
	state->rflags = rflags_int;
	state->rip = 0; // to be filled by the loader
	state->user_rsp = page_manager::initial_stack;

	// Next state in the stack is the loader's kernel stack. The scheduler will
	// iret to this which will kick off the loading:
	cpu_state *loader_state = reinterpret_cast<cpu_state *>(sp0) - 2;

	loader_state->ds = loader_state->ss = kss;
	loader_state->cs = kcs;
	loader_state->rflags = rflags_noint;
	loader_state->rip = reinterpret_cast<uint64_t>(ramdisk_process_loader);
	loader_state->user_rsp = reinterpret_cast<uint64_t>(state);

	loader_state->rax = reinterpret_cast<uint64_t>(data);
	loader_state->rbx = size;

	uint16_t pid = m_next_pid++;
	auto *proc = m_process_allocator.pop();

	proc->pid = pid;
	proc->ppid = 0; // TODO
	proc->priority = default_priority;
	proc->rsp = reinterpret_cast<addr_t>(loader_state);
	proc->pml4 = pml4;
	proc->flags =
		process_flags::ready |
		process_flags::loading;

	m_runlists[default_priority].push_back(proc);

	loader_state->rcx = reinterpret_cast<uint64_t>(proc);

	log::debug(logs::task, "Creating process %s: pid %d  pri %d", name, proc->pid, proc->priority);
	log::debug(logs::task, "     RSP0 %016lx", state);
	log::debug(logs::task, "     PML4 %016lx", pml4);
}

void
scheduler::start()
{
	log::info(logs::task, "Starting scheduler.");
	m_apic->enable_timer(isr::isrTimer, 128, quantum, false);
}

addr_t
scheduler::schedule(addr_t rsp0)
{
	m_current->rsp = rsp0;
	m_runlists[m_current->priority].remove(m_current);

	if (m_current->flags && process_flags::ready)
		m_runlists[m_current->priority].push_back(m_current);
	else
		m_blocked.push_back(m_current);

	uint8_t pri = 0;
	while (m_runlists[pri].empty()) {
		++pri;
		kassert(pri < num_priorities, "All runlists are empty");
	}

	m_current = m_runlists[pri].pop_front();
	rsp0 = m_current->rsp;

	// Set rsp0 to after the end of the about-to-be-popped cpu state
	tss_set_stack(0, rsp0 + sizeof(cpu_state));
	wrmsr(msr::ia32_kernel_gs_base, rsp0);
	log::debug(logs::task, "Scheduler set kernel_gs_base to %016lx", rsp0);

	// Swap page tables
	page_table *pml4 = m_current->pml4;
	page_manager::set_pml4(pml4);

	bool loading = m_current->flags && process_flags::loading;
	log::debug(logs::task, "Scheduler switched to process %d, priority %d%s.",
			m_current->pid, m_current->priority, loading ? " (loading)" : "");

	return rsp0;
}

addr_t
scheduler::tick(addr_t rsp0)
{
	// TODO: action based on the task using the whole quantum

	rsp0 = schedule(rsp0);
	m_apic->reset_timer(quantum);
	return rsp0;
}
