#pragma once
/// \file memory_pages.h
/// Structures related to handling memory paging.

#include <stdint.h>

#include "kutil/enum_bitfields.h"

/// Flags used by `page_block`.
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


/// A block of contiguous pages. Each `page_block` represents contiguous
/// physical pages with the same attributes. A `page_block *` is also a
/// linked list of such structures.
struct page_block
{
	uint64_t physical_address;
	uint64_t virtual_address;
	uint32_t count;
	page_block_flags flags;
	page_block *next;

	bool has_flag(page_block_flags f) const { return bitfield_contains(flags, f); }
	uint64_t physical_end() const { return physical_address + (count * 0x1000); }
	uint64_t virtual_end() const { return virtual_address + (count * 0x1000); }

	/// Traverse the list, joining adjacent blocks where possible.
	/// \returns  A linked list of freed page_block structures.
	page_block * list_consolidate();

	/// Traverse the list, printing debug info on this list.
	/// \arg name  [optional] String to print as the name of this list
	void list_dump(const char *name = nullptr);
};


/// Helper struct for computing page table indices of a given address.
struct page_table_indices
{
	page_table_indices(uint64_t v = 0) :
		index{
			(v >> 39) & 0x1ff,
			(v >> 30) & 0x1ff,
			(v >> 21) & 0x1ff,
			(v >> 12) & 0x1ff }
	{}

	/// Get the index for a given level of page table.
	uint64_t & operator[](size_t i) { return index[i]; }
	uint64_t index[4]; ///< Indices for each level of tables.
};

/// Calculate a page-aligned address.
/// \arg p    The address to align.
/// \returns  The next page-aligned address _after_ `p`.
template <typename T> inline T page_align(T p)       { return ((p - 1) & ~0xfffull) + 0x1000; }

/// Calculate a page-table-aligned address. That is, an address that is
/// page-aligned to the first page in a page table.
/// \arg p    The address to align.
/// \returns  The next page-table-aligned address _after_ `p`.
template <typename T> inline T page_table_align(T p) { return ((p - 1) & ~0x1fffffull) + 0x200000; }
