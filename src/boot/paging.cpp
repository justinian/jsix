#include "console.h"
#include "error.h"
#include "loader.h"
#include "memory.h"
#include "paging.h"
#include "pointer_manipulation.h"

namespace boot {
namespace paging {

using memory::page_size;

void allocate_tables(kernel::args::header *args, uefi::boot_services *bs)
{
	status_line status(L"Allocating initial page tables");

	static constexpr size_t offset_map_tables = 128 + 1;
	static constexpr size_t tables_needed = offset_map_tables + 49;

	void *addr = nullptr;
	try_or_raise(
		bs->allocate_pages(
			uefi::allocate_type::any_pages,
			memory::table_type,
			tables_needed,
			&addr),
		L"Error allocating page table pages.");

	bs->set_mem(addr, tables_needed*page_size, 0);

	kernel::args::module &mod = args->modules[++args->num_modules];
	mod.type = kernel::args::mod_type::page_tables;
	mod.location = addr;
	mod.size = tables_needed*page_size;

	args->pml4 = addr;
	args->num_free_tables = tables_needed - offset_map_tables;
	args->page_table_cache = offset_ptr<void>(addr, offset_map_tables*page_size);

	page_table *tables = reinterpret_cast<page_table*>(addr);

	// Create the PML4 pointing to the following tables
	for (int i = 0; i < offset_map_tables - 1; ++i) {
		tables[0].set(384 + i, &tables[i+1], 0x0003);

		uint64_t start = i * 0x8000000000;
		for (int j = 0; j < 512; ++j)
		{
			void *p = reinterpret_cast<void*>(start + (j * 0x40000000ull));
			tables[i+1].set(j, p, 0x0183);
		}
	}
}

void
check_needs_page(page_table *table, int idx, kernel::args::header *args)
{
	if (table->entries[idx] & 0x1)
		return;

	uintptr_t new_table =
		reinterpret_cast<uintptr_t>(args->page_table_cache);
	table->entries[idx] = new_table | 0x0003;

	args->page_table_cache = offset_ptr<void>(args->page_table_cache, page_size);
	args->num_free_tables--;
}

void
map_in(
	page_table *pml4,
	kernel::args::header *args,
	uintptr_t phys, uintptr_t virt,
	size_t size)
{
	page_table_indices idx{virt};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	size_t pages = memory::bytes_to_pages(size);

	for (; idx[0] < 512; idx[0] += 1, idx[1] = 0, idx[2] = 0, idx[3] = 0) {
		check_needs_page(tables[0], idx[0], args);
		tables[1] = tables[0]->get(idx[0]);

		for (; idx[1] < 512; idx[1] += 1, idx[2] = 0, idx[3] = 0) {
			check_needs_page(tables[1], idx[1], args);
			tables[2] = tables[1]->get(idx[1]);

			for (; idx[2] < 512; idx[2] += 1, idx[3] = 0) {
				check_needs_page(tables[2], idx[2], args);
				tables[3] = tables[2]->get(idx[2]);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys | 0x003;
					phys += page_size;
					if (--pages == 0) return;
				}
			}
		}
	}
}


page_table_indices::page_table_indices(uint64_t v) :
	index{
		(v >> 39) & 0x1ff,
		(v >> 30) & 0x1ff,
		(v >> 21) & 0x1ff,
		(v >> 12) & 0x1ff }
{}

uintptr_t
page_table_indices::addr() const
{
	return
		(index[0] << 39) |
		(index[1] << 30) |
		(index[2] << 21) |
		(index[3] << 12);
}

bool operator==(const page_table_indices &l, const page_table_indices &r)
{
	return l[0] == r[0] && l[1] == r[1] && l[2] == r[2] && l[3] == r[3];
}
} // namespace paging
} // namespace boot
