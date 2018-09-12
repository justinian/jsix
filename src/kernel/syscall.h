#pragma once

enum class syscall : uint64_t
{
	noop,
	debug,
	message,

	last_syscall
};
