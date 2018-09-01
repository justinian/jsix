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

extern "C" void taskA();
extern "C" void taskB();


scheduler::scheduler(lapic *apic) :
	m_apic(apic),
	m_current(0)
{
	m_processes.ensure_capacity(50);
}

static process
create_process(uint16_t pid, void (*rip)())
{
	uint64_t flags;
	__asm__ __volatile__ ( "pushf; pop %0" : "=r" (flags) );

	uint16_t kcs = (1 << 3) | 0; // Kernel CS is GDT entry 1, ring 0
	uint16_t cs = (5 << 3) | 3;  // User CS is GDT entry 5, ring 3

	uint16_t kss = (2 << 3) | 0; // Kernel SS is GDT entry 2, ring 0
	uint16_t ss = (4 << 3) | 3;  // User SS is GDT entry 4, ring 3

	void *stack0 = kutil::malloc(stack_size);
	void *sp0 = kutil::offset_pointer(stack0, stack_size);
	cpu_state *state = reinterpret_cast<cpu_state *>(sp0) - 1;
	kutil::memset(state, 0, sizeof(cpu_state));

	state->ds = state->ss = ss;
	state->cs = cs;
	state->rflags = 0x202;
	state->rip = reinterpret_cast<uint64_t>(rip);

	page_table *pml4 = page_manager::get()->create_process_map();
	state->user_rsp = page_manager::initial_stack;

	log::debug(logs::task, "Creating PID %d:", pid);
	log::debug(logs::task, "  RSP0 %016lx", state);
	log::debug(logs::task, "  RSP3 %016lx", state->user_rsp);
	log::debug(logs::task, "  PML4 %016lx", pml4);

	return {pid, reinterpret_cast<addr_t>(state), pml4};
}

void
scheduler::start()
{
	log::info(logs::task, "Starting scheduler.");

	m_apic->enable_timer(isr::isrTimer, 128, quantum, false);

	m_processes.append({0, 0, page_manager::get_pml4()}); // The kernel idle task, also the thread we're in now
	m_processes.append(create_process(1, &taskA));
	m_processes.append(create_process(2, &taskB));
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
