#pragma once

#include <stdint.h>

struct cpu_state;

enum class syscall : uint64_t
{
#define SYSCALL(name, nargs) name ,
#include "syscalls.inc"
#undef SYSCALL

	COUNT
};

void syscall_enable();
extern "C" void syscall_invalid(uint64_t call);
