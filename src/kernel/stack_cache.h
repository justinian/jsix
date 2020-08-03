#pragma once

#include <kutil/vector.h>

/// A cache of kernel stack address ranges
class stack_cache
{
public:
	/// Constructor.
	/// \args start  Start of virtual memory area to contain stacks
	/// \args size   Size of virtual memory area in bytes
	stack_cache(uintptr_t start, size_t size);

	/// Get an available stack address
	uintptr_t get_stack();

	/// Return a stack address to the available pool
	void return_stack(uintptr_t addr);

	static stack_cache & get() { return s_instance; }

private:
	kutil::vector<uintptr_t> m_cache;
	uintptr_t m_next;
	const uintptr_t m_end;
	static stack_cache s_instance;
};
