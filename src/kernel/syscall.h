#pragma once

#include <stdint.h>
#include "j6/types.h"

struct cpu_state;

enum class syscall : uint64_t
{
#define SYSCALL(id, name, ...) name = id,
#include "syscalls.inc"
#undef SYSCALL

	// Maximum syscall id. If you change this, also change
	// MAX_SYSCALLS in syscall.s
	MAX = 0x40
};

void syscall_enable();

namespace syscalls
{
#define SYSCALL(id, name, ...) j6_status_t name (__VA_ARGS__);
#include "syscalls.inc"
#undef SYSCALL
}
