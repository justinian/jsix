#pragma once

#include <stdint.h>

struct cpu_state;

enum class syscall : uint64_t
{
	noop,
	debug,
	message,
	pause,
	sleep,

	last_syscall
};

void syscall_enable();
uintptr_t syscall_dispatch(uintptr_t, const cpu_state &);

