#pragma once

#include <stdint.h>

#include "kutil/enum_bitfields.h"

enum class page_block_flags : uint32_t
{
	// Not a flag value, but for comparison
	free         = 0x00000000,

	used         = 0x00000001,
	mapped       = 0x00000002,
	pending_free = 0x00000004,

	nonvolatile  = 0x00000010,
	acpi_wait    = 0x00000020,

	permanent    = 0x80000000,

	max_flags
};
IS_BITFIELD(page_block_flags);

struct page_block
{
	uint64_t physical_address;
	uint64_t virtual_address;
	uint32_t count;
	page_block_flags flags;
	page_block *next;

	bool has_flag(page_block_flags f) const { return bitfield_contains(flags, f); }
	uint64_t end() const { return physical_address + (count * 0x1000); }

	page_block * list_consolidate();
	void list_dump(const char *name = nullptr);
};

struct page_table_indices
{
	page_table_indices(uint64_t v) :
		index{
			(v >> 39) & 0x1ff,
			(v >> 30) & 0x1ff,
			(v >> 21) & 0x1ff,
			(v >> 12) & 0x1ff }
	{}

	uint64_t & operator[](size_t i) { return index[i]; }
	uint64_t index[4];

	void dump();
};
