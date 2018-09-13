#pragma once

#include <stdint.h>
#include "kutil/memory.h"

enum class syscall : uint64_t
{
	noop,
	debug,
	message,
	pause,

	last_syscall
};

struct cpu_state;
addr_t syscall_dispatch(addr_t, const cpu_state &);

