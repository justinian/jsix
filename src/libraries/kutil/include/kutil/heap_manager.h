#pragma once
/// \file heap_manager.h
/// A buddy allocator and related definitions.

#include <stddef.h>

namespace kutil {


/// Manager for allocation of heap memory.
class heap_manager
{
public:
	/// Callback signature for growth function. Memory returned does not need
	/// to be contiguous, but needs to be alined to the length requested.
	using grow_callback = void * (*)(size_t length);

	/// Default constructor. Creates an invalid manager.
	heap_manager();

	/// Constructor.
	/// \arg grow_cb  Function pointer to grow the heap size
	heap_manager(grow_callback grow_cb);

	/// Allocate memory from the area managed.
	/// \arg length  The amount of memory to allocate, in bytes
	/// \returns     A pointer to the allocated memory, or nullptr if
	///              allocation failed.
	void * allocate(size_t length);

	/// Free a previous allocation.
	/// \arg p  A pointer previously retuned by allocate()
	void free(void *p);

	/// Minimum block size is (2^min_size). Must be at least 6.
	static const unsigned min_size = 6;

	/// Maximum block size is (2^max_size). Must be less than 64.
	static const unsigned max_size = 16;

protected:
	class mem_header;

	/// Expand the size of memory
	void grow_memory();

	/// Ensure there is a block of a given size, recursively splitting
	/// \arg size   Size category of the block we want
	void ensure_block(unsigned size);

	/// Helper accessor for the list of blocks of a given size
	/// \arg size   Size category of the block we want
	/// \returns    A mutable reference to the head of the list
	mem_header *& get_free(unsigned size)  { return m_free[size - min_size]; }

	/// Helper to get a block of the given size, growing if necessary
	/// \arg size   Size category of the block we want
	/// \returns    A detached block of the given size
	mem_header * pop_free(unsigned size);

	mem_header *m_free[max_size - min_size + 1];

	grow_callback m_grow;

	heap_manager(const heap_manager &) = delete;
};

} // namespace kutil
