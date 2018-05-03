#pragma once
/// \file memory.h
/// The block memory manager and related definitions.

#include <stddef.h>
#include <stdint.h>


/// Manager for allocation of virtual memory.
class memory_manager
{
public:
	memory_manager();
	memory_manager(void *start);

	/// Allocate memory from the area managed.
	/// \arg length  The amount of memory to allocate, in bytes
	/// \returns     A pointer to the allocated memory, or nullptr if
	///              allocation failed.
	void * allocate(size_t length);

	/// Free a previous allocation.
	/// \arg p  A pointer previously retuned by allocate()
	void free(void *p);

private:
	struct alloc_header;
	struct free_header;

	/// Expand the size of memory
	void grow_memory();

	/// Ensure there is a block of a given size, recursively splitting
	/// \arg size   Size category of the block we want
	void ensure_block(unsigned size);

	/// Helper accessor for the list of blocks of a given size
	free_header *& get_free(unsigned size)  { return m_free[size - min_size]; }

	/// Helper to get a block of the given size, growing if necessary
	free_header * pop_free(unsigned size);

	static const unsigned min_size = 6;  ///< Minimum block size is (2^min_size)
	static const unsigned max_size = 16; ///< Maximum block size is (2^max_size)

	free_header *m_free[max_size - min_size];
	void *m_start;
	size_t m_length;

	friend class page_manager;
	memory_manager(const memory_manager &) = delete;
};

extern memory_manager g_kernel_memory_manager;

/// Bootstrap the memory managers.
void memory_initialize_managers(const void *memory_map, size_t map_length, size_t desc_length);

/// Allocate kernel space memory.
/// \arg length  The amount of memory to allocate, in bytes
/// \returns     A pointer to the allocated memory, or nullptr if
///              allocation failed.
inline void * kalloc(size_t length) { return g_kernel_memory_manager.allocate(length); }

/// Free kernel space memory.
/// \arg p   The pointer to free
inline void kfree(void *p) { g_kernel_memory_manager.free(p); }

inline void * operator new (size_t n)    { return g_kernel_memory_manager.allocate(n); }
inline void * operator new[] (size_t n)  { return g_kernel_memory_manager.allocate(n); }
inline void operator delete (void *p)  { return g_kernel_memory_manager.free(p); }
inline void operator delete[] (void *p){ return g_kernel_memory_manager.free(p); }
