#pragma once
/// \file paging.h
/// Page table structure and related definitions
#include <stdint.h>
#include <uefi/boot_services.h>

namespace bootproto {
    struct args;
}

namespace boot {
namespace paging {

struct page_table;

class pager
{
public:
    static constexpr size_t pd_tables = 256;    // number of pages for kernelspace PDs
    static constexpr size_t extra_tables = 64;  // number of extra pages

    pager(uefi::boot_services *bs);

    /// Map physical memory pages to virtual addresses in the given page tables.
    /// \arg phys       The physical address of the pages to map
    /// \arg virt       The virtual address at which to map the pages
    /// \arg count      The number of pages to map
    /// \arg write_flag If true, mark the pages writeable
    /// \arg exe_flag   If true, mark the pages executable
    void map_pages(
        uintptr_t phys,
        uintptr_t virt,
        size_t count,
        bool write_flag,
        bool exe_flag);

    /// Update the kernel args structure before handing it off to the kernel
    void update_kernel_args(bootproto::args *args);

    /// Copy existing page table entries from the UEFI PML4 into our new PML4.
    /// Does not do a deep copy - the new PML4 is updated to point to the
    /// existing next-level page tables in the current PML4.
    void add_current_mappings();

    /// Write the pager's PML4 pointer to CR3
    inline void install() const {
        asm volatile ( "mov %0, %%cr3" :: "r" (m_pml4) );
        __sync_synchronize();
    }

private:
    template <unsigned D>
    friend class page_entry_iterator;

    /// Get `count` table pages from the cache
    page_table * pop_pages(size_t count);

    /// Allocate the kernel PML4 and PD tables out of the cache, and add the
    /// linear offset mappings to them
    void create_kernel_tables();

    uefi::boot_services *m_bs;
    util::counted<void> m_table_pages;
    page_table *m_pml4;
    page_table *m_kernel_pds;
};

} // namespace paging
} // namespace boot
