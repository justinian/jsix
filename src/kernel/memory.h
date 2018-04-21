#pragma once

#include <stddef.h>

struct page_block;

class memory_manager
{
public:

	static void create(const void *memory_map, size_t map_length, size_t desc_length);
	static memory_manager * get() { return s_instance; }

private:
	memory_manager(page_block *free, page_block *used, void *scratch, size_t scratch_len);

	memory_manager() = delete;
	memory_manager(const memory_manager &) = delete;

	static memory_manager * s_instance;
};
