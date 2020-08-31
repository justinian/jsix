#pragma once

#include <kutil/vector.h>

/// A cache of kernel stack address ranges
class buffer_cache
{
public:
	/// Constructor.
	/// \args start  Start of virtual memory area to contain buffers
	/// \args size   Size of individual buffers in bytes
	/// \args length Size of virtual memory area in bytes
	buffer_cache(uintptr_t start, size_t size, size_t length);

	/// Get an available stack address
	uintptr_t get_buffer();

	/// Return a buffer address to the available pool
	void return_buffer(uintptr_t addr);

private:
	kutil::vector<uintptr_t> m_cache;
	uintptr_t m_next;
	const uintptr_t m_end;
	const size_t m_size;
};

extern buffer_cache g_kstack_cache;
extern buffer_cache g_kbuffer_cache;
