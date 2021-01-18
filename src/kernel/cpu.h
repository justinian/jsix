#pragma once

#include <stdint.h>

struct TCB;
class thread;
class process;

struct cpu_state
{
	uint64_t r15, r14, r13, r12, r11, r10, r9,  r8;
	uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
	uint64_t interrupt, errorcode;
	uint64_t rip, cs, rflags, user_rsp, ss;
};

/// Per-cpu state data. If you change this, remember to update the assembly
/// version in 'tasking.inc'
struct cpu_data
{
	uintptr_t rsp0;
	uintptr_t rsp3;
	TCB *tcb;
	thread *t;
	process *p;
};

extern cpu_data bsp_cpu_data;

// We already validated the required options in the bootloader,
// but iterate the options and log about them.
void cpu_validate();
