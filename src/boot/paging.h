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
/// \arg args    The kernel args struct, used for the page table cache and pml4
void allocate_tables(kernel::init::args *args);

/// Copy existing page table entries to a new page table. Does not do a deep
/// copy - the new PML4 is updated to point to the existing next-level page
/// tables in the current PML4.
/// \arg new_pml4  The new PML4 to copy into
void add_current_mappings(page_table *new_pml4);

/// Map physical memory pages to virtual addresses in the given page tables.
/// \arg args       The kernel args struct, used for the page table cache and pml4
/// \arg phys       The physical address of the pages to map
/// \arg virt       The virtual address at which to map the pages
/// \arg count      The number of pages to map
/// \arg write_flag If true, mark the pages writeable
/// \arg exe_flag   If true, mark the pages executable
void map_pages(
    kernel::init::args *args,
    uintptr_t phys, uintptr_t virt,
    size_t count, bool write_flag, bool exe_flag);

/// Map the sections of a program in physical memory to their virtual memory
/// addresses in the given page tables.
/// \arg args    The kernel args struct, used for the page table cache and pml4
/// \arg program The program to load
void map_program(
    kernel::init::args *args,
    kernel::init::program &program);

} // namespace paging
} // namespace boot
