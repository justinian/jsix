#pragma once
/// \file page_manager.h
/// The page memory manager and related definitions.

#include <stddef.h>
#include <stdint.h>

#include "kutil/enum_bitfields.h"
#include "kutil/linked_list.h"
#include "kutil/memory.h"
#include "frame_allocator.h"
#include "kernel_memory.h"
#include "page_table.h"

struct free_page_header;

/// Manager for allocation and mapping of pages
class page_manager
{
public:
	/// Constructor.
	/// \arg frames  The frame allocator to get physical frames from
	/// \arg pml4    The initial kernel-space pml4
	page_manager(frame_allocator &frames, page_table *pml4);

	/// Helper to get the number of pages needed for a given number of bytes.
	/// \arg bytes  The number of bytes desired
	/// \returns    The number of pages needed to contain the desired bytes
	static inline size_t page_count(size_t bytes)
	{
		return (bytes - 1) / memory::frame_size + 1;
	}

	/// Helper to read the PML4 table from CR3.
	/// \returns  A pointer to the current PML4 table.
	static inline page_table * get_pml4()
	{
		uintptr_t pml4 = 0;
		__asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (pml4) );
		return reinterpret_cast<page_table *>((pml4 & ~0xfffull) + memory::page_offset);
	}

	/// Helper to set the PML4 table pointer in CR3.
	/// \arg pml4  A pointer to the PML4 table to install.
	static inline void set_pml4(page_table *pml4)
	{
		constexpr uint64_t phys_mask = ~memory::page_offset & ~0xfffull;
		uintptr_t p = reinterpret_cast<uintptr_t>(pml4) & phys_mask;
		__asm__ __volatile__ ( "mov %0, %%cr3" :: "r" (p) );
	}

	/// Allocate and map pages into virtual memory.
	/// \arg address  The virtual address at which to map the pages
	/// \arg count    The number of pages to map
	/// \arg user     True is this memory is user-accessible
	/// \arg pml4     The pml4 to map into - null for the current one
	/// \returns      A pointer to the start of the mapped region
	void * map_pages(uintptr_t address, size_t count, bool user = false, page_table *pml4 = nullptr);

	/// Unmap and free existing pages from memory.
	/// \arg address  The virtual address of the memory to unmap
	/// \arg count    The number of pages to unmap
	/// \arg pml4     The pml4 to unmap from - null for the current one
	void unmap_pages(void *address, size_t count, page_table *pml4 = nullptr);

	/// Dump the given or current PML4 to the console
	/// \arg pml4     The page table to use, null for the current one
	/// \arg recurse  Whether to print sub-tables
	void dump_pml4(page_table *pml4 = nullptr, bool recurse = true);

	/// Get the system page manager.
	/// \returns  A pointer to the system page manager
	static page_manager * get();

	/// Get a pointer to the kernel's PML4
	inline page_table * get_kernel_pml4() { return m_kernel_pml4; }

private:
	/// Helper function to allocate a new page table. If table entry `i` in
	/// table `base` is empty, allocate a new page table and point `base[i]` at
	/// it.
	/// \arg base  Existing page table being indexed into
	/// \arg i     Index into the existing table to check
	/// \art user  True if this is a userspace mapping
	void check_needs_page(page_table *base, unsigned i, bool user);

	/// Low-level routine for mapping a number of pages into the given page table.
	/// \arg pml4       The root page table to map into
	/// \arg phys_addr  The starting physical address of the pages to be mapped
	/// \arg virt_addr  The starting virtual address ot the memory to be mapped
	/// \arg count      The number of pages to map
	/// \arg user       True if this is a userspace mapping
	/// \arg large      Whether to allow large pages
	void page_in(
			page_table *pml4,
			uintptr_t phys_addr,
			uintptr_t virt_addr,
			size_t count,
			bool user = false,
			bool large = false);

	/// Low-level routine for unmapping a number of pages from the given page table.
	/// \arg pml4       The root page table for this mapping
	/// \arg virt_addr  The starting virtual address ot the memory to be unmapped
	/// \arg count      The number of pages to unmap
	/// \arg free       Whether to return the pages to the frame allocator
	void page_out(
			page_table *pml4,
			uintptr_t virt_addr,
			size_t count,
			bool free = false);

	/// Low-level routine for unmapping an entire table of memory at once
	void unmap_table(page_table *table, page_table::level lvl, bool free,
			page_table_indices index = {});

	page_table *m_kernel_pml4; ///< The PML4 of just kernel pages

	frame_allocator &m_frames;

	friend class memory_bootstrap;
	friend class vm_space;
	page_manager(const page_manager &) = delete;
};

/// Global page manager.
extern page_manager &g_page_manager;

inline page_manager * page_manager::get() { return &g_page_manager; }


/// Calculate a page-aligned address.
/// \arg p    The address to align.
/// \returns  The next page-aligned address _after_ `p`.
template <typename T> inline T
page_align(T p)
{
	return reinterpret_cast<T>(
		((reinterpret_cast<uintptr_t>(p) - 1) & ~(memory::frame_size - 1))
		+ memory::frame_size);
}

/// Calculate a page-table-aligned address. That is, an address that is
/// page-aligned to the first page in a page table.
/// \arg p    The address to align.
/// \returns  The next page-table-aligned address _after_ `p`.
template <typename T> inline T
page_table_align(T p)
{
	return ((p - 1) & ~0x1fffffull) + 0x200000;
}
