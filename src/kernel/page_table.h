#pragma once
/// \file page_table.h
/// Helper structures for dealing with page tables.

#include <stdint.h>
#include "kernel_memory.h"

class page_manager;

/// Struct to allow easy accessing of a memory page being used as a page table.
struct page_table
{
	enum class level : unsigned { pml4, pdp, pd, pt };
	inline static level deeper(level l) {
		return static_cast<level>(static_cast<unsigned>(l) + 1);
	}

	uint64_t entries[512];

	inline page_table * get(int i, uint16_t *flags = nullptr) const {
		uint64_t entry = entries[i];
		if ((entry & 0x1) == 0) return nullptr;
		if (flags) *flags = entry & 0xfffull;
		return reinterpret_cast<page_table *>((entry & ~0xfffull) + memory::page_offset);
	}

	inline void set(int i, page_table *p, uint16_t flags) {
		entries[i] = (reinterpret_cast<uint64_t>(p) - memory::page_offset) | (flags & 0xfff);
	}

	inline bool is_present(int i) const { return (entries[i] & 0x1) == 0x1; }

	inline bool is_large_page(level l, int i) const {
		return
			(l == level::pdp || l == level::pd) &&
			(entries[i] & 0x80) == 0x80;
	}

	void dump(
		level lvl = level::pml4,
		bool recurse = true);
};


/// Helper struct for computing page table indices of a given address.
struct page_table_indices
{
	page_table_indices(uint64_t v = 0);

	uintptr_t addr() const;

	inline operator uintptr_t() const { return addr(); }

	/// Get the index for a given level of page table.
	uint64_t & operator[](int i) { return index[i]; }
	uint64_t operator[](int i) const { return index[i]; }
	uint64_t & operator[](page_table::level i) { return index[static_cast<unsigned>(i)]; }
	uint64_t operator[](page_table::level i) const { return index[static_cast<unsigned>(i)]; }
	uint64_t index[4]; ///< Indices for each level of tables.
};

bool operator==(const page_table_indices &l, const page_table_indices &r);

