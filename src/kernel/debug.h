#pragma once
/// \file debug.h
/// Debugging utilities

#include "kutil/memory.h"

extern "C" {
	addr_t get_rsp();
	addr_t get_rip();
	addr_t get_frame(int frame);

}

void print_regs(const cpu_state &regs);
void print_stack(const cpu_state &regs);
void print_stacktrace(int skip = 0);

#define print_reg(name, value) cons->printf("         %s: %016lx\n", name, (value));

