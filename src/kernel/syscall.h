#pragma once

#include <stdint.h>
#include "kutil/memory.h"

struct cpu_state;

enum class syscall : uint64_t
{
	noop,
	debug,
	message,
	pause,

	last_syscall
};

void syscall_enable();
addr_t syscall_dispatch(addr_t, const cpu_state &);

