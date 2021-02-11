#pragma once

#include <stdint.h>

#include "kutil/spinlock.h"

class GDT;
class process;
struct TCB;
class thread;
class TSS;

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
	cpu_data *self;
	uint16_t id;
	uint16_t index;
	uint32_t reserved;
	uintptr_t rsp0;
	uintptr_t rsp3;
	TCB *tcb;
	thread *thread;
	process *process;
	TSS *tss;
	GDT *gdt;

};

extern "C" cpu_data * _current_gsbase();

/// Set up the running CPU. This sets GDT, IDT, and necessary MSRs as well as creating
/// the cpu_data structure for this processor.
/// \arg cpu  The cpu_data structure for this CPU
/// \arg bsp  True if this CPU is the BSP
void cpu_init(cpu_data *cpu, bool bsp);

/// Do early (before cpu_init) initialization work. Only needs to be called manually for
/// the BSP, otherwise cpu_init will call it.
/// \arg cpu  The cpu_data structure for this CPU
void cpu_early_init(cpu_data *cpu);

/// Get the cpu_data struct for the current executing CPU
inline cpu_data & current_cpu() { return *_current_gsbase(); }

/// Validate the required CPU features are present. Really, the bootloader already
/// validated the required features, but still iterate the options and log about them.
void cpu_validate();

