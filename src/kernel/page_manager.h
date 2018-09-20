#pragma once
/// \file page_manager.h
/// The page memory manager and related definitions.

#include <stddef.h>
#include <stdint.h>

#include "kutil/enum_bitfields.h"
#include "kutil/linked_list.h"
#include "kutil/slab_allocator.h"

struct page_block;
struct page_table;
struct free_page_header;

using page_block_list = kutil::linked_list<page_block>;
using page_block_slab = kutil::slab_allocator<page_block>;

/// Manager for allocation of physical pages.
class page_manager
{
public:
	/// Size of a single page.
	static const size_t page_size = 0x1000;

	/// Start of the higher half.
	static const uintptr_t high_offset = 0xffffff0000000000;

	/// Offset from physical where page tables are mapped.
	static const uintptr_t page_offset = 0xffffff8000000000;

	/// Initial process thread's stack address
	static const uintptr_t initial_stack = 0x0000800000000000;

	/// Initial process thread's stack size, in pages
	static const unsigned initial_stack_pages = 1;

	page_manager();

	/// Helper to get the number of pages needed for a given number of bytes.
	/// \arg bytes  The number of bytes desired
	/// \returns    The number of pages needed to contain the desired bytes
	static inline size_t page_count(size_t bytes)
	{
		return (bytes - 1) / page_size + 1;
	}

	/// Helper to read the PML4 table from CR3.
	/// \returns  A pointer to the current PML4 table.
	static inline page_table * get_pml4()
	{
		uintptr_t pml4 = 0;
		__asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (pml4) );
		return reinterpret_cast<page_table *>((pml4 & ~0xfffull) + page_offset);
	}

	/// Helper to set the PML4 table pointer in CR3.
	/// \arg pml4  A pointer to the PML4 table to install.
	static inline void set_pml4(page_table *pml4)
	{
		uintptr_t p = reinterpret_cast<uintptr_t>(pml4) - page_offset;
		__asm__ __volatile__ ( "mov %0, %%cr3" :: "r" (p & ~0xfffull) );
	}

	/// Allocate but don't switch to a new PML4 table. This table
	/// should only have global kernel pages mapped.
	/// \returns      A pointer to the PML4 table
	page_table * create_process_map();

	/// Deallocate a process' PML4 table.
	void delete_process_map(page_table *table);

	/// Allocate and map pages into virtual memory.
	/// \arg address  The virtual address at which to map the pages
	/// \arg count    The number of pages to map
	/// \arg user     True is this memory is user-accessible
	/// \arg pml4     The pml4 to map into - null for the current one
	/// \returns      A pointer to the start of the mapped region
	void * map_pages(uintptr_t address, size_t count, bool user = false, page_table *pml4 = nullptr);

	/// Allocate and map contiguous pages into virtual memory, with
	/// a constant offset from their physical address.
	/// \arg count    The number of pages to map
	/// \returns      A pointer to the start of the mapped region, or
	/// nullptr if no region could be found to fit the request.
	void * map_offset_pages(size_t count);

	/// Unmap existing pages from memory.
	/// \arg address  The virtual address of the memory to unmap
	/// \arg count    The number of pages to unmap
	void unmap_pages(void *address, size_t count);

	/// Offset-map a pointer. No physical pages will be mapped.
	/// \arg pointer  Pointer to a pointer to the memory area to be mapped
	/// \arg length   Length of the memory area to be mapped
	void map_offset_pointer(void **pointer, size_t length);

	/// Get the physical address of an offset-mapped pointer
	/// \arg p   Virtual address of memory that has been offset-mapped
	/// \returns Physical address of the memory pointed to by p
	inline uintptr_t offset_phys(void *p) const
	{
		return reinterpret_cast<uintptr_t>(kutil::offset_pointer(p, -page_offset));
	}

	/// Get the virtual address of an offset-mapped physical address
	/// \arg a   Physical address of memory that has been offset-mapped
	/// \returns Virtual address of the memory at address a
	inline void * offset_virt(uintptr_t a) const
	{
		return kutil::offset_pointer(reinterpret_cast<void *>(a), page_offset);
	}

	/// Log the current free/used block lists.
	/// \arg used_only  If true, skip printing free list. Default false.
	void dump_blocks(bool used_only = false);

	/// Dump the given or current PML4 to the console
	/// \arg pml4       The page table to use, null for the current one
	/// \arg max_index  The max index of pml4 to print
	void dump_pml4(page_table *pml4 = nullptr, int max_index = 511);

	/// Get the system page manager.
	/// \returns  A pointer to the system page manager
	static page_manager * get();

private:
	/// Set up the memory manager from bootstraped memory
	void init(
			page_block_list free,
			page_block_list used,
			page_block_list cache);

	/// Initialize the virtual memory manager based on this object's state
	void init_memory_manager();

	/// Create a `page_block` struct or pull one from the cache.
	/// \returns  An empty `page_block` struct
	page_block * get_block();

	/// Return a list of `page_block` structs to the cache.
	/// \arg block   A list of `page_block` structs
	void free_blocks(page_block *block);

	/// Allocate a page for a page table, or pull one from the cache
	/// \returns  An empty page mapped in page space
	page_table * get_table_page();

	/// Return a set of mapped contiguous pages to the page cache.
	/// \arg pages  Pointer to the first page to be returned
	/// \arg count  Number of pages in the range
	void free_table_pages(void *pages, size_t count);

	/// Consolidate the free and used block lists. Return freed blocks
	/// to the cache.
	void consolidate_blocks();

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
	/// \art user  True if this is a userspace mapping
	void page_in(
			page_table *pml4,
			uintptr_t phys_addr,
			uintptr_t virt_addr,
			size_t count,
			bool user = false);

	/// Low-level routine for unmapping a number of pages from the given page table.
	/// \arg pml4       The root page table for this mapping
	/// \arg virt_addr  The starting virtual address ot the memory to be unmapped
	/// \arg count      The number of pages to unmap
	void page_out(
			page_table *pml4,
			uintptr_t virt_addr,
			size_t count);

	/// Get free pages from the free list. Only pages from the first free block
	/// are returned, so the number may be less than requested, but they will
	/// be contiguous. Pages will not be mapped into virtual memory.
	/// \arg count    The maximum number of pages to get
	/// \arg address  [out] The address of the first page
	/// \returns      The number of pages retrieved
	size_t pop_pages(size_t count, uintptr_t *address);

	page_table *m_kernel_pml4; ///< The PML4 of just kernel pages

	page_block_list m_free; ///< Free pages list
	page_block_list m_used; ///< In-use pages list
	page_block_slab m_block_slab; ///< page_block slab allocator

	free_page_header *m_page_cache; ///< Cache of free pages to use for tables

	friend void memory_initialize(const void *, size_t, size_t);
	page_manager(const page_manager &) = delete;
};

/// Global page manager.
extern page_manager g_page_manager;

inline page_manager * page_manager::get() { return &g_page_manager; }

/// Flags used by `page_block`.
enum class page_block_flags : uint32_t
{
	free         = 0x00000000,  ///< Not a flag, value for free memory
	used         = 0x00000001,  ///< Memory is in use
	mapped       = 0x00000002,  ///< Memory is mapped to virtual address

	mmio         = 0x00000010,  ///< Memory is a MMIO region
	nonvolatile  = 0x00000020,  ///< Memory is non-volatile storage

	pending_free = 0x10000000,  ///< Memory should be freed
	acpi_wait    = 0x40000000,  ///< Memory should be freed after ACPI init
	permanent    = 0x80000000,  ///< Memory is permanently unusable

	max_flags
};
IS_BITFIELD(page_block_flags);


/// A block of contiguous pages. Each `page_block` represents contiguous
/// physical pages with the same attributes. A `page_block *` is also a
/// linked list of such structures.
struct page_block
{
	uintptr_t physical_address;
	uintptr_t virtual_address;
	uint32_t count;
	page_block_flags flags;

	inline bool has_flag(page_block_flags f) const { return bitfield_has(flags, f); }
	inline uintptr_t physical_end() const { return physical_address + (count * page_manager::page_size); }
	inline uintptr_t virtual_end() const { return virtual_address + (count * page_manager::page_size); }

	inline bool contains(uintptr_t vaddr) const { return vaddr >= virtual_address && vaddr < virtual_end(); }
	inline bool contains_physical(uintptr_t addr) const { return addr >= physical_address && addr < physical_end(); }

	/// Helper to zero out a block and optionally set the next pointer.
	void zero();

	/// Helper to copy a bock from another block
	/// \arg other  The block to copy from
	void copy(page_block *other);

	/// Compare two blocks by address.
	/// \arg rhs   The right-hand comparator
	/// \returns   <0 if this is sorts earlier, >0 if this sorts later, 0 for equal
	int compare(const page_block *rhs) const;

	/// Traverse the list, joining adjacent blocks where possible.
	/// \arg list  The list to consolidate
	/// \returns   A linked list of freed page_block structures.
	static page_block_list consolidate(page_block_list &list);

	/// Traverse the list, printing debug info on this list.
	/// \arg list  The list to print
	/// \arg name  [optional] String to print as the name of this list
	/// \arg show_permanent [optional] If false, hide unmapped blocks
	static void dump(const page_block_list &list, const char *name = nullptr, bool show_unmapped = false);
};



/// Struct to allow easy accessing of a memory page being used as a page table.
struct page_table
{
	using pm = page_manager;

	uint64_t entries[512];

	inline page_table * get(int i) const {
		uint64_t entry = entries[i];
		if ((entry & 0x1) == 0) return nullptr;
		return reinterpret_cast<page_table *>((entry & ~0xfffull) + pm::page_offset);
	}

	inline void set(int i, page_table *p, uint16_t flags) {
		entries[i] = (reinterpret_cast<uint64_t>(p) - pm::page_offset) | (flags & 0xfff);
	}

	void dump(int level = 4, int max_index = 511, uint64_t offset = page_manager::page_offset);
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
template <typename T> inline T
page_align(T p)
{
	return reinterpret_cast<T>(
		((reinterpret_cast<uintptr_t>(p) - 1) & ~(page_manager::page_size - 1))
		+ page_manager::page_size);
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


/// Calculate the number of pages needed for the give number of bytes.
/// \arg n   Number of bytes
/// \returns Number of pages
inline size_t page_count(size_t n) { return ((n - 1) / page_manager::page_size) + 1; }


/// Bootstrap the memory managers.
void memory_initialize(const void *memory_map, size_t map_length, size_t desc_length);
