#pragma once

#include <stdint.h>

struct cpu_state;

enum class syscall : uint64_t
{
#define SYSCALL(id, name, result, ...) name = id,
#include "syscalls.inc"
#undef SYSCALL

	// Maximum syscall id. If you change this, also change
	// MAX_SYSCALLS in syscall.s
	MAX = 64
};

void syscall_enable();

namespace syscalls
{
#define SYSCALL(id, name, result, ...) result name (__VA_ARGS__);
#include "syscalls.inc"
#undef SYSCALL
}
