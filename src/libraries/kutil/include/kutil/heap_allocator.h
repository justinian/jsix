#pragma once
/// \file heap_allocator.h
/// A buddy allocator for a memory heap

#include <stddef.h>

namespace kutil {


/// Allocator for a given heap range
class heap_allocator
{
public:
	/// Default constructor creates a valid but empty heap.
	heap_allocator();

	/// Constructor. The given memory area must already have been reserved.
	/// \arg start  Starting address of the heap
	/// \arg size   Size of the heap in bytes
	heap_allocator(uintptr_t start, size_t size);

	/// Allocate memory from the area managed.
	/// \arg length  The amount of memory to allocate, in bytes
	/// \returns     A pointer to the allocated memory, or nullptr if
	///              allocation failed.
	virtual void * allocate(size_t length);

	/// Free a previous allocation.
	/// \arg p  A pointer previously retuned by allocate()
	virtual void free(void *p);

	/// Minimum block size is (2^min_size). Must be at least 6.
	static const unsigned min_size = 6;

	/// Maximum block size is (2^max_size). Must be less than 64.
	static const unsigned max_size = 22;

protected:
	class mem_header;

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

	uintptr_t m_next;
	size_t m_size;
	mem_header *m_free[max_size - min_size + 1];

	heap_allocator(const heap_allocator &) = delete;
};

} // namespace kutil
