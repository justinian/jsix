#include "kernel_memory.h"

#include "console.h"
#include "error.h"
#include "loader.h"
#include "memory.h"
#include "paging.h"
#include "pointer_manipulation.h"
#include "status.h"

namespace boot {
namespace paging {

using memory::page_size;
using ::memory::pml4e_kernel;
using ::memory::table_entries;

// Flags: 0 0 0 1  0 0 0 0  0 0 1 1 = 0x0103
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
constexpr uint16_t page_flags = 0x103;

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
constexpr uint16_t huge_page_flags = 0x183;

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
constexpr uint16_t table_flags = 0x003;

/// Iterator over page table entries.
template <unsigned D = 4>
class page_entry_iterator
{
public:
	/// Constructor.
	/// \arg virt       Virtual address this iterator is starting at
	/// \arg pml4       Root of the page tables to iterate
	/// \arg page_cache Pointer to pages that can be used for page tables
	/// \arg page_count Number of pages pointed to by `page_cache`
	page_entry_iterator(
			uintptr_t virt,
			page_table *pml4,
			void *&page_cache,
			size_t &cache_count) :
		m_page_cache(page_cache),
		m_cache_count(cache_count)
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
			if (!m_cache_count--)
				error::raise(uefi::status::out_of_resources, L"Page table cache empty");

			page_table *table = reinterpret_cast<page_table*>(m_page_cache);
			m_page_cache = offset_ptr<void>(m_page_cache, page_size);

			parent_ent = (reinterpret_cast<uintptr_t>(table) & ~0xfffull) | table_flags;
			m_table[level] = table;
		} else {
			m_table[level] = reinterpret_cast<page_table*>(parent_ent & ~0xfffull);
		}
	}

	void *&m_page_cache;
	size_t &m_cache_count;
	page_table *m_table[D];
	uint16_t m_index[D];
};


static void
add_offset_mappings(page_table *pml4, void *&page_cache, size_t &num_pages)
{
	uintptr_t phys = 0;
	uintptr_t virt = ::memory::page_offset; // Start of offset-mapped area
	size_t pages = 64 * 1024; // 64 TiB of 1 GiB pages
	constexpr size_t GiB = 0x40000000ull;

	page_entry_iterator<2> iterator{
		virt, pml4,
		page_cache,
		num_pages};

	while (true) {
		*iterator = phys | huge_page_flags;
		if (--pages == 0)
			break;

		iterator.increment();
		phys += GiB;
	}
}

static void
add_kernel_pds(page_table *pml4, void *&page_cache, size_t &num_pages)
{
	for (unsigned i = pml4e_kernel; i < table_entries; ++i) {
		pml4->set(i, page_cache, table_flags);
		page_cache = offset_ptr<void>(page_cache, page_size);
		num_pages--;
	}
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
allocate_tables(kernel::args::header *args, uefi::boot_services *bs)
{
	status_line status(L"Allocating initial page tables");

	static constexpr size_t pd_tables = 256;   // number of pages for kernelspace PDs
	static constexpr size_t extra_tables = 49; // number of extra pages

	// number of pages for kernelspace PDs + PML4
	static constexpr size_t kernel_tables = pd_tables + 1;

	static constexpr size_t tables_needed = kernel_tables + extra_tables;

	void *addr = nullptr;
	try_or_raise(
		bs->allocate_pages(
			uefi::allocate_type::any_pages,
			uefi::memory_type::loader_data,
			tables_needed,
			&addr),
		L"Error allocating page table pages.");

	bs->set_mem(addr, tables_needed*page_size, 0);

	page_table *pml4 = reinterpret_cast<page_table*>(addr);

	args->pml4 = pml4;
	args->table_count = tables_needed - 1;
	args->page_tables = offset_ptr<void>(addr, page_size);

	add_kernel_pds(pml4, args->page_tables, args->table_count);
	add_offset_mappings(pml4, args->page_tables, args->table_count);

	console::print(L"    Set up initial mappings, %d spare tables.\r\n", args->table_count);
}

void
map_pages(
	kernel::args::header *args,
	uintptr_t phys, uintptr_t virt,
	size_t size)
{
	paging::page_table *pml4 =
		reinterpret_cast<paging::page_table*>(args->pml4);

	size_t pages = memory::bytes_to_pages(size);
	page_entry_iterator<4> iterator{
		virt, pml4,
		args->page_tables,
		args->table_count};

	while (true) {
		*iterator = phys | page_flags;
		if (--pages == 0)
			break;

		iterator.increment();
		phys += page_size;
	}
}


} // namespace paging
} // namespace boot
