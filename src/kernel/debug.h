#pragma once
/// \file debug.h
/// Debugging utilities

#include <stdint.h>

struct cpu_state;

extern "C" {
    uintptr_t get_rsp();
    uintptr_t get_rip();
    uintptr_t get_caller();
    uintptr_t get_grandcaller();
    uintptr_t get_frame(int frame);
    uintptr_t get_gsbase();
    void _halt();
}

extern size_t __counter_syscall_enter;
extern size_t __counter_syscall_sysret;

void print_regs(const cpu_state &regs);
void print_stack(const cpu_state &regs);

#define print_regL(name, value) cons->printf("       %s: %016lx", name, (value));
#define print_regM(name, value) cons->printf("    %s: %016lx", name, (value));
#define print_regR(name, value) cons->printf("    %s: %016lx\n", name, (value));

