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

	/// Minimum block size is (2^min_order). Must be at least 6.
	static const unsigned min_order = 6;

	/// Maximum block size is (2^max_order). Must be less than 64.
	static const unsigned max_order = 22;

protected:
	class mem_header;

	/// Ensure there is a block of a given order, recursively splitting
	/// \arg order   Order (2^N) of the block we want
	void ensure_block(unsigned order);

	/// Helper accessor for the list of blocks of a given order
	/// \arg order  Order (2^N) of the block we want
	/// \returns    A mutable reference to the head of the list
	mem_header *& get_free(unsigned order)  { return m_free[order - min_order]; }

	/// Helper to get a block of the given order, growing if necessary
	/// \arg order  Order (2^N) of the block we want
	/// \returns    A detached block of the given order
	mem_header * pop_free(unsigned order);

	uintptr_t m_next;
	size_t m_size;
	mem_header *m_free[max_order - min_order + 1];
	size_t m_allocated_size;

	heap_allocator(const heap_allocator &) = delete;
};

} // namespace kutil
