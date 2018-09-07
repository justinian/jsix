#pragma once

enum class syscall : uint64_t
{
	debug,
	message,

	last_syscall
};
