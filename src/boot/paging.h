#pragma once
/// \file paging.h
/// Page table structure and related definitions
#include <stdint.h>
#include <uefi/boot_services.h>
#include "kernel_args.h"

namespace boot {
namespace paging {

/// Struct to allow easy accessing of a memory page being used as a page table.
struct page_table
{
	uint64_t entries[512];

	inline page_table * get(int i, uint16_t *flags = nullptr) const {
		uint64_t entry = entries[i];
		if ((entry & 1) == 0) return nullptr;
		if (flags) *flags = entry & 0xfff;
		return reinterpret_cast<page_table *>(entry & ~0xfffull);
	}

	inline void set(int i, void *p, uint16_t flags) {
		entries[i] = reinterpret_cast<uint64_t>(p) | (flags & 0xfff);
	}
};

/// Allocate memory to be used for initial page tables. Initial offset-mapped
/// page tables are pre-filled. All pages are saved as a module in kernel args
/// and kernel args' `page_table_cache` and `num_free_tables` are updated with
/// the leftover space.
void allocate_tables(
	kernel::args::header *args,
	uefi::boot_services *bs);

/// Copy existing page table entries to a new page table. Does not do a deep
/// copy - the new PML4 is updated to point to the existing next-level page
/// tables in the current PML4.
void add_current_mappings(page_table *new_pml4);

/// Map a program section in physical memory to its virtual address in the
/// given page tables.
/// \arg args    The kernel args header, used for the page table cache and pml4
/// \arg section The program section to load
void map_section(
	kernel::args::header *args,
	const kernel::args::program_section &section);


} // namespace paging
} // namespace boot
