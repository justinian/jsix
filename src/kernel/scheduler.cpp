#include "apic.h"
#include "console.h"
#include "cpu.h"
#include "gdt.h"
#include "interrupts.h"
#include "log.h"
#include "page_manager.h"
#include "scheduler.h"

scheduler scheduler::s_instance(nullptr);
//static const uint32_t quantum = 2000000;
static const uint32_t quantum =  20000000;

const int stack_size = 0x1000;
const uint64_t rflags_noint = 0x002;
const uint64_t rflags_int = 0x202;

extern "C" {
	void taskA();
	void taskAend();
	void ramdisk_process_loader();

	void load_process(void *copy_start, size_t butes, cpu_state state);
};

scheduler::scheduler(lapic *apic) :
	m_apic(apic),
	m_current(0)
{
	m_processes.ensure_capacity(50);
}

void
load_process(void *copy_start, size_t bytes, cpu_state state)
{
	log::debug(logs::task, "Loading task! Start: %016lx [%d]", copy_start, bytes);
	log::debug(logs::task, "New process state:");
	log::debug(logs::task, "        CS: %d [%d]", state.cs >> 3, state.cs & 0x07);
	log::debug(logs::task, "        SS: %d [%d]", state.ss >> 3, state.ss & 0x07);
	log::debug(logs::task, "    RFLAGS: %08x", state.rflags);
	log::debug(logs::task, "       RIP: %016lx", state.rip);
	log::debug(logs::task, "      uRSP: %016lx", state.user_rsp);

	// We're now in the process space for this process, allocate memory for the
	// process code and load it
	page_manager *pager = page_manager::get();

	size_t count = ((bytes - 1) / page_manager::page_size) + 1;
	void *loading = pager->map_pages(0x200000, count, true);

	kutil::memcpy(loading, copy_start, bytes);
	state.rip = reinterpret_cast<uint64_t>(loading);
}

static process
create_process(uint16_t pid)
{
	uint64_t flags;
	__asm__ __volatile__ ( "pushf; pop %0" : "=r" (flags) );

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

	// TODO: replace with an actual ELF location in memory
	loader_state->rax = reinterpret_cast<uint64_t>(taskA);
	loader_state->rbx = reinterpret_cast<uint64_t>(taskAend) - reinterpret_cast<uint64_t>(taskA);

	log::debug(logs::task, "Creating PID %d:", pid);
	log::debug(logs::task, "     RSP0 %016lx", state);
	log::debug(logs::task, "     RSP3 %016lx", state->user_rsp);
	log::debug(logs::task, "     PML4 %016lx", pml4);
	log::debug(logs::task, "  Loading %016lx [%d]", loader_state->rax, loader_state->rbx);

	return {pid, reinterpret_cast<addr_t>(loader_state), pml4};
}

void
scheduler::start()
{
	log::info(logs::task, "Starting scheduler.");

	m_apic->enable_timer(isr::isrTimer, 128, quantum, false);

	m_processes.append({0, 0, page_manager::get_pml4()}); // The kernel idle task, also the thread we're in now
	m_processes.append(create_process(1));
	//m_processes.append(create_process(2, &taskB));
}

addr_t
scheduler::tick(addr_t rsp0)
{
	log::debug(logs::task, "Scheduler tick.");

	m_processes[m_current].rsp = rsp0;
	m_current = (m_current + 1) % m_processes.count();
	rsp0 = m_processes[m_current].rsp;

	// Set rsp0 to after the end of the about-to-be-popped cpu state
	tss_set_stack(0, rsp0 + sizeof(cpu_state));

	// Swap page tables
	page_table *pml4 = m_processes[m_current].pml4;
	page_manager::set_pml4(pml4);

	m_apic->reset_timer(quantum);
	return rsp0;
}
