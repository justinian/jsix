#pragma once
/// \file memory.h
/// The block memory manager and related definitions.

#include <stddef.h>

/// Manager for allocation of virtual memory.
class memory_manager
{
public:
	memory_manager();

	memory_manager(const memory_manager &) = delete;

private:
	friend class page_manager;
};

extern memory_manager g_memory_manager;

/// Bootstrap the memory managers.
void memory_initialize_managers(const void *memory_map, size_t map_length, size_t desc_length);

