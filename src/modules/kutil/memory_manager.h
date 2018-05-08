#pragma once
/// \file memory_manager.h
/// A buddy allocator and related definitions.

#include <stddef.h>

namespace kutil {


/// Manager for allocation of virtual memory.
class memory_manager
{
public:
	using grow_callback = void (*)(void *start, size_t length);

	/// Default constructor. Creates an invalid manager.
	memory_manager();

	/// Constructor.
	/// \arg start    Pointer to the start of the heap to be managed
	/// \arg grow_cb  Function pointer to grow the heap size
	memory_manager(void *start, grow_callback grow_cb);

	/// Allocate memory from the area managed.
	/// \arg length  The amount of memory to allocate, in bytes
	/// \returns     A pointer to the allocated memory, or nullptr if
	///              allocation failed.
	void * allocate(size_t length);

	/// Free a previous allocation.
	/// \arg p  A pointer previously retuned by allocate()
	void free(void *p);


private:
	class mem_header;

	/// Expand the size of memory
	void grow_memory();

	/// Ensure there is a block of a given size, recursively splitting
	/// \arg size   Size category of the block we want
	void ensure_block(unsigned size);

	/// Helper accessor for the list of blocks of a given size
	mem_header *& get_free(unsigned size)  { return m_free[size - min_size]; }

	/// Helper to get a block of the given size, growing if necessary
	mem_header * pop_free(unsigned size);

	/// Minimum block size is (2^min_size). Must be at least 6.
	static const unsigned min_size = 6;

	/// Maximum block size is (2^max_size). Must be less than 64.
	static const unsigned max_size = 16;

	mem_header *m_free[max_size - min_size];
	void *m_start;
	size_t m_length;

	grow_callback m_grow;

	memory_manager(const memory_manager &) = delete;
};

} // namespace kutil
