#pragma once
/// \file debug.h
/// Debugging utilities

#include <stdint.h>

extern "C" {
	uintptr_t get_rsp();
	uintptr_t get_rip();
	uintptr_t get_frame(int frame);
	uintptr_t get_gsbase();
}

extern size_t __counter_syscall_enter;
extern size_t __counter_syscall_sysret;

void print_regs(const cpu_state &regs);
void print_stack(const cpu_state &regs);
void print_stacktrace(int skip);

#define print_reg(name, value) cons->printf("         %s: %016lx\n", name, (value));

