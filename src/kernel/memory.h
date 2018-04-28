#pragma once
/// \file memory.h
/// The block memory manager and related definitions.

#include <stddef.h>
#include <stdint.h>

struct memory_block_node;

/// Manager for allocation of virtual memory.
class memory_manager
{
public:
	memory_manager();
	memory_manager(void *start, size_t length);

	/// Allocate memory from the area managed.
	/// \arg length  The amount of memory to allocate, in bytes
	/// \returns     A pointer to the allocated memory, or nullptr if
	///              allocation failed.
	void * allocate(size_t length);

	/// Free a previous allocation.
	/// \arg p  A pointer previously retuned by allocate()
	void free(void *p);

private:
	friend class page_manager;

	// Simple incrementing pointer.
	void *m_start;
	size_t m_length;

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
