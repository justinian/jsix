#pragma once

#include <stdint.h>

struct cpu_state;

enum class syscall : uint64_t
{
	noop           = 0x0000,
	debug          = 0x0001,
	message        = 0x0002,
	pause          = 0x0003,
	sleep          = 0x0004,
	getpid         = 0x0005,
	send           = 0x0006,
	receive        = 0x0007,
	fork           = 0x0008,
	exit           = 0x0009,

	last_syscall
};

void syscall_enable();
void syscall_dispatch(cpu_state *);

