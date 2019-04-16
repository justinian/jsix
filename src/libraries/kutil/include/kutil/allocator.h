#pragma once
/// \file allocator.h
/// Allocator interface

#include <stdint.h>
#include "kernel_memory.h"

namespace kutil {

class allocator
{
public:
	/// Allocate memory.
	/// \arg length  The amount of memory to allocate, in bytes
	/// \returns     A pointer to the allocated memory, or nullptr if
	///              allocation failed.
	virtual void * allocate(size_t size) = 0;

	/// Free a previous allocation.
	/// \arg p  A pointer previously retuned by allocate()
	virtual void free(void *p) = 0;

	template <typename T>
	inline T * allocate(unsigned count) {
		return reinterpret_cast<T*>(allocate(count * sizeof(T)));
	}

	static allocator &invalid;
};

} // namespace kutil
