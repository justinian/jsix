#include "kernel_memory.h"

#include "allocator.h"
#include "console.h"
#include "error.h"
#include "loader.h"
#include "memory.h"
#include "paging.h"
#include "pointer_manipulation.h"
#include "status.h"

namespace boot {
namespace paging {

using memory::alloc_type;
using memory::page_size;
using ::memory::pml4e_kernel;
using ::memory::table_entries;

// Flags: 0 0 0 1  0 0 0 0  0 0 0 1 = 0x0101
//         IGN  |  | | | |  | | | +- Present
//              |  | | | |  | | +--- Writeable
//              |  | | | |  | +----- Usermode access (supervisor only)
//              |  | | | |  +------- PWT (determining memory type for page)
//              |  | | | +---------- PCD (determining memory type for page)
//              |  | | +------------ Accessed flag (not accessed yet)
//              |  | +-------------- Dirty (not dirtied yet)
//              |  +---------------- PAT (determining memory type for page)
//              +------------------- Global
/// Page table entry flags for entries pointing at a page
constexpr uint64_t page_flags = 0x101;

// Flags: 0  0 0 0 1  1 0 0 0  1 0 1 1 = 0x018b
//        |   IGN  |  | | | |  | | | +- Present
//        |        |  | | | |  | | +--- Writeable
//        |        |  | | | |  | +----- Supervisor only
//        |        |  | | | |  +------- PWT (determining memory type for page)
//        |        |  | | | +---------- PCD (determining memory type for page)
//        |        |  | | +------------ Accessed flag (not accessed yet)
//        |        |  | +-------------- Dirty (not dirtied yet)
//        |        |  +---------------- Page size (1GiB page)
//        |        +------------------- Global
//        +---------------------------- PAT (determining memory type for page)
/// Page table entry flags for entries pointing at a huge page
constexpr uint64_t huge_page_flags = 0x18b;

// Flags: 0 0 0 0  0 0 0 0  0 0 1 1 = 0x0003
//        IGNORED  | | | |  | | | +- Present
//                 | | | |  | | +--- Writeable
//                 | | | |  | +----- Usermode access (Supervisor only)
//                 | | | |  +------- PWT (determining memory type for pdpt)
//                 | | | +---------- PCD (determining memory type for pdpt)
//                 | | +------------ Accessed flag (not accessed yet)
//                 | +-------------- Ignored
//                 +---------------- Reserved 0 (Table pointer, not page)
/// Page table entry flags for entries pointing at another table
constexpr uint64_t table_flags = 0x003;


inline void *
pop_pages(counted<void> &pages, size_t count)
{
	if (count > pages.count)
		error::raise(uefi::status::out_of_resources, L"Page table cache empty", 0x7ab1e5);

	void *next = pages.pointer;
	pages.pointer = offset_ptr<void>(pages.pointer, count*page_size);
	pages.count -= count;
	return next;
}

/// Iterator over page table entries.
template <unsigned D = 4>
class page_entry_iterator
{
public:
	/// Constructor.
	/// \arg virt    Virtual address this iterator is starting at
	/// \arg pml4    Root of the page tables to iterate
	/// \arg pages   Cache of usable table pages
	page_entry_iterator(
			uintptr_t virt,
			page_table *pml4,
			counted<void> &pages) :
		m_pages(pages)
	{
		m_table[0] = pml4;
		for (unsigned i = 0; i < D; ++i) {
			m_index[i] = static_cast<uint16_t>((virt >> (12 + 9*(3-i))) & 0x1ff);
			ensure_table(i);
		}
	}

	uintptr_t vaddress() const {
		uintptr_t address = 0;
		for (unsigned i = 0; i < D; ++i)
			address |= static_cast<uintptr_t>(m_index[i]) << (12 + 9*(3-i));
		if (address & (1ull<<47)) // canonicalize the address
			address |= (0xffffull<<48);
		return address;
	}

	void increment()
	{
		for (unsigned i = D - 1; i >= 0; --i) {
			if (++m_index[i] <= 511) {
				for (unsigned j = i + 1; j < D; ++j)
					ensure_table(j);
				return;
			}

			m_index[i] = 0;
		}
	}

	uint64_t & operator*() { return entry(D-1); }

private:
	inline uint64_t & entry(unsigned level) { return m_table[level]->entries[m_index[level]]; }

	void ensure_table(unsigned level)
	{
		// We're only dealing with D levels of paging, and
		// there must always be a PML4.
		if (level < 1 || level >= D)
			return;

		// Entry in the parent that points to the table we want
		uint64_t & parent_ent = entry(level - 1);

		if (!(parent_ent & 1)) {
			page_table *table = reinterpret_cast<page_table*>(pop_pages(m_pages, 1));
			parent_ent = (reinterpret_cast<uintptr_t>(table) & ~0xfffull) | table_flags;
			m_table[level] = table;
		} else {
			m_table[level] = reinterpret_cast<page_table*>(parent_ent & ~0xfffull);
		}
	}

	counted<void> &m_pages;
	page_table *m_table[D];
	uint16_t m_index[D];
};


static void
add_offset_mappings(page_table *pml4, counted<void> &pages)
{
	uintptr_t phys = 0;
	uintptr_t virt = ::memory::page_offset; // Start of offset-mapped area
	size_t page_count = 64 * 1024; // 64 TiB of 1 GiB pages
	constexpr size_t GiB = 0x40000000ull;

	page_entry_iterator<2> iterator{virt, pml4, pages};

	while (true) {
		*iterator = phys | huge_page_flags;
		if (--page_count == 0)
			break;

		iterator.increment();
		phys += GiB;
	}
}

static void
add_kernel_pds(page_table *pml4, counted<void> &pages)
{
	for (unsigned i = pml4e_kernel; i < table_entries; ++i)
		pml4->set(i, pop_pages(pages, 1), table_flags);
}

void
add_current_mappings(page_table *new_pml4)
{
	// Get the pointer to the current PML4
	page_table *old_pml4 = 0;
	asm volatile ( "mov %%cr3, %0" : "=r" (old_pml4) );

	// Only copy mappings in the lower half
	for (int i = 0; i < ::memory::pml4e_kernel; ++i) {
		uint64_t entry = old_pml4->entries[i];
		if (entry & 1)
			new_pml4->entries[i] = entry;
	}
}

void
allocate_tables(kernel::init::args *args)
{
	status_line status(L"Allocating initial page tables");

	static constexpr size_t pd_tables = 256;   // number of pages for kernelspace PDs
	static constexpr size_t extra_tables = 64; // number of extra pages

	// number of pages for kernelspace PDs + PML4
	static constexpr size_t kernel_tables = pd_tables + 1;

	static constexpr size_t tables_needed = kernel_tables + extra_tables;

	void *addr = g_alloc.allocate_pages(tables_needed, alloc_type::page_table, true);
	page_table *pml4 = reinterpret_cast<page_table*>(addr);

	args->pml4 = pml4;
	args->page_tables = { .pointer = pml4 + 1, .count = tables_needed - 1 };

	console::print(L"    First page (pml4) at: 0x%lx\r\n", pml4);

	add_kernel_pds(pml4, args->page_tables);
	add_offset_mappings(pml4, args->page_tables);

	//console::print(L"    Set up initial mappings, %d spare tables.\r\n", args->table_count);
}

template <typename E>
constexpr bool has_flag(E set, E flag) {
	return
		(static_cast<uint64_t>(set) & static_cast<uint64_t>(flag)) ==
		 static_cast<uint64_t>(flag);
}

void
map_pages(
	kernel::init::args *args,
	uintptr_t phys, uintptr_t virt,
	size_t count, bool write_flag, bool exe_flag)
{
	if (!count)
		return;

	paging::page_table *pml4 =
		reinterpret_cast<paging::page_table*>(args->pml4);

	page_entry_iterator<4> iterator{virt, pml4, args->page_tables};

	uint64_t flags = page_flags;
	if (!exe_flag)
		flags |= (1ull << 63); // set NX bit
	if (write_flag)
		flags |= 2;

	while (true) {
		*iterator = phys | flags;
		if (--count == 0)
			break;

		iterator.increment();
		phys += page_size;
	}
}

void
map_section(
	kernel::init::args *args,
	const kernel::init::program_section &section)
{
	using kernel::init::section_flags;

	size_t pages = memory::bytes_to_pages(section.size);

	map_pages(
		args,
		section.phys_addr,
		section.virt_addr,
		pages,
		has_flag(section.type, section_flags::write),
		has_flag(section.type, section_flags::execute));
}


} // namespace paging
} // namespace boot
