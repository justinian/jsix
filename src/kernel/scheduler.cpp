#include "apic.h"
#include "console.h"
#include "cpu.h"
#include "gdt.h"
#include "interrupts.h"
#include "log.h"
#include "scheduler.h"

scheduler scheduler::s_instance(nullptr);
static const uint32_t quantum = 5000000;

const int stack_size = 0x1000;
char taskAstack[stack_size];
char taskBstack[stack_size];

uint64_t taskAcount = 0;

void taskA()
{
	console *cons = console::get();
	while(1) {
		cons->putc('.');
	}
}

void taskB()
{
	console *cons = console::get();
	while(1) {
		cons->putc('+');
	}
}


scheduler::scheduler(lapic *apic) :
	m_apic(apic),
	m_current(0)
{
	m_processes.ensure_capacity(50);
}

static process
create_process(uint16_t pid, void *stack, void (*rip)())
{
	uint64_t flags;
	__asm__ __volatile__ ( "pushf; pop %0" : "=r" (flags) );

	// This is a hack for now, until we get a lot more set up.
	// I just want to see task switching working inside ring0 first
	uint16_t kcs = (1 << 3) | 0;
	uint16_t cs = (3 << 3) | 3;

	uint16_t kss = (2 << 3) | 0;
	uint16_t ss = (4 << 3) | 3;

	void *sp = kutil::offset_pointer(stack, stack_size);
	cpu_state *state = reinterpret_cast<cpu_state *>(sp) - 1;
	kutil::memset(state, 0, sizeof(cpu_state));
	state->ds = state->ss = kss;
	state->cs = kcs;
	state->rflags = 0x202;
	state->user_rsp = reinterpret_cast<uint64_t>(sp);
	state->rip = reinterpret_cast<uint64_t>(rip);

	log::debug(logs::task, "Creating a user RSP of %016lx", state->user_rsp);

	return {pid, reinterpret_cast<addr_t>(state)};
}

void
scheduler::start()
{
	m_apic->enable_timer(isr::isrTimer, 128, quantum, false);

	m_processes.append({0, 0}); // The kernel idle task
	m_processes.append(create_process(1, &taskAstack[0], &taskA));
	m_processes.append(create_process(2, &taskBstack[0], &taskB));
}

addr_t
scheduler::tick(addr_t rsp0)
{
	log::debug(logs::task, "Scheduler tick.");

	m_processes[m_current].rsp = rsp0;
	m_current = (m_current + 1) % m_processes.count();
	rsp0 = m_processes[m_current].rsp;
	tss_set_stack(0, rsp0);

	m_apic->reset_timer(quantum);
	return rsp0;
}
