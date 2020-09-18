#include "kutil/assert.h"
#include "console.h"
#include "io.h"
#include "log.h"
#include "objects/process.h"
#include "objects/vm_area.h"
#include "page_manager.h"
#include "vm_space.h"

using memory::frame_size;
using memory::heap_start;
using memory::kernel_max_heap;
using memory::kernel_offset;
using memory::page_offset;
using memory::page_mappable;
using memory::pml4e_kernel;
using memory::table_entries;

// NB: in 4KiB page table entries, bit 7 isn't pagesize but PAT. Currently this
// doesn't matter, becasue in the default PAT table, both 000 and 100 are WB.
constexpr uint64_t sys_page_flags   = 0x183; // global, pagesize, write, present
constexpr uint64_t sys_table_flags  = 0x003; // write, present
constexpr uint64_t user_page_flags  = 0x087; // pagesize, user, write, present
constexpr uint64_t user_table_flags = 0x007; // user, write, present

static uintptr_t
pt_to_phys(page_table *pt)
{
	return reinterpret_cast<uintptr_t>(pt) - page_offset;
}


static page_table *
pt_from_phys(uintptr_t p)
{
	return reinterpret_cast<page_table *>((p + page_offset) & ~0xfffull);
}




page_manager::page_manager(frame_allocator &frames, page_table *pml4) :
	m_kernel_pml4(pml4),
	m_frames(frames)
{
}

page_table *
page_manager::create_process_map()
{
	page_table *table = page_table::get_table_page();

	kutil::memset(table, 0, frame_size/2);
	for (unsigned i = pml4e_kernel; i < table_entries; ++i)
		table->entries[i] = m_kernel_pml4->entries[i];

	return table;
}

void
page_manager::delete_process_map(page_table *pml4)
{
	bool was_pml4 = (pml4 == get_pml4());
	if (was_pml4)
		set_pml4(m_kernel_pml4);

	log::info(logs::paging, "Deleting process pml4 at %016lx%s",
			pml4, was_pml4 ? " (was current)" : "");

	unmap_table(pml4, page_table::level::pml4, true);
}

void *
page_manager::get_offset_from_mapped(void *p, page_table *pml4)
{
	if (!pml4) pml4 = get_pml4();
	uintptr_t v = reinterpret_cast<uintptr_t>(p);

	page_table_indices idx{v};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	for (int i = 1; i < 4; ++i) {
		tables[i] = tables[i-1]->get(idx[i-1]);
		if (!tables[i])
			return nullptr;
	}

	uintptr_t a = tables[3]->entries[idx[3]];
	if (!(a & 1))
		return nullptr;

	return memory::to_virtual<void>(
			(a & ~0xfffull) |
			(v & 0xfffull));
}

void
page_manager::dump_pml4(page_table *pml4, bool recurse)
{
	if (pml4 == nullptr) pml4 = get_pml4();
	pml4->dump(page_table::level::pml4, recurse);
}

void *
page_manager::map_pages(uintptr_t address, size_t count, bool user, page_table *pml4)
{
	kassert(address, "Cannot call map_pages with 0 address");

	void *ret = reinterpret_cast<void *>(address);
	if (!pml4) pml4 = get_pml4();

	while (count) {
		uintptr_t phys = 0;
		size_t n = m_frames.allocate(count, &phys);

		log::info(logs::paging, "Paging in %d pages at p:%016lx to v:%016lx into %016lx table",
				n, phys, address, pml4);

		page_in(pml4, phys, address, n, user);

		address += n * frame_size;
		count -= n;
	}

	return ret;
}

void
page_manager::unmap_table(page_table *table, page_table::level lvl, bool free, page_table_indices index)
{
	const int max =
		lvl == page_table::level::pml4
			? pml4e_kernel
			: table_entries;

	uintptr_t free_start = 0;
	uintptr_t free_start_virt = 0;
	uintptr_t free_count = 0;

	size_t size =
		lvl == page_table::level::pdp ? (1<<30) :
		lvl == page_table::level::pd  ? (1<<21) :
		lvl == page_table::level::pt  ? (1<<12) :
		0;

	for (int i = 0; i < max; ++i) {
		if (!table->is_present(i)) continue;

		index[lvl] = i;

		bool is_page =
			lvl == page_table::level::pt ||
			table->is_large_page(lvl, i);

		if (is_page) {
			uintptr_t frame = table->entries[i] & ~0xfffull;
			if (!free_count || frame != free_start + free_count * size) {
				if (free_count && free) {
					log::debug(logs::paging,
							"  freeing v:%016lx-%016lx p:%016lx-%016lx",
							free_start_virt, free_start_virt + free_count * frame_size,
							free_start, free_start + free_count * frame_size);

					m_frames.free(free_start, (free_count * size) / frame_size);
					free_count = 0;
				}

				if (!free_count) {
					free_start = frame;
					free_start_virt = index.addr();
				}
			}

			free_count += 1;
		} else {
			page_table *next = table->get(i);
			unmap_table(next, page_table::deeper(lvl), free, index);
		}
	}

	if (free_count && free) {
		log::debug(logs::paging,
				"  freeing v:%016lx-%016lx p:%016lx-%016lx",
				free_start_virt, free_start_virt + free_count * frame_size,
				free_start, free_start + free_count * frame_size);

		m_frames.free(free_start, (free_count * size) / frame_size);
	}
	page_table::free_table_page(table);

	log::debug(logs::paging, "Unmapped%s lv %d table at %016lx",
			free ? " (and freed)" : "", lvl, table);
}

void
page_manager::unmap_pages(void* address, size_t count, page_table *pml4)
{
	if (!pml4)
		pml4 = get_pml4();

	uintptr_t iaddr = reinterpret_cast<uintptr_t>(address);

	page_out(pml4, iaddr, count, true);
}

void
page_manager::check_needs_page(page_table *table, unsigned index, bool user)
{
	if ((table->entries[index] & 0x1) == 1) return;

	page_table *new_table = page_table::get_table_page();
	for (int i=0; i<table_entries; ++i) new_table->entries[i] = 0;
	table->entries[index] = pt_to_phys(new_table) | (user ? user_table_flags : sys_table_flags);
}

void
page_manager::page_in(page_table *pml4, uintptr_t phys_addr, uintptr_t virt_addr, size_t count, bool user, bool large)
{
	/*
	log::debug(logs::paging, "page_in for table %016lx p:%016lx v:%016lx c:%4d u:%d l:%d",
			pml4, phys_addr, virt_addr, count, user, large);
	*/

	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	uint64_t flags = user ? user_table_flags : sys_table_flags;

	for (; idx[0] < table_entries; idx[0] += 1) {
		check_needs_page(tables[0], idx[0], user);
		tables[1] = tables[0]->get(idx[0]);

		for (; idx[1] < table_entries; idx[1] += 1, idx[2] = 0, idx[3] = 0) {
			check_needs_page(tables[1], idx[1], user);
			tables[2] = tables[1]->get(idx[1]);

			for (; idx[2] < table_entries; idx[2] += 1, idx[3] = 0) {
				if (large &&
					idx[3] == 0 &&
					count >= table_entries &&
					tables[2]->get(idx[2]) == nullptr) {
					// Do a 2MiB page instead
					tables[2]->entries[idx[2]] = phys_addr | flags | 0x80;
					phys_addr += frame_size * table_entries;
					count -= table_entries;
					if (count == 0) return;
					continue;
				}

				check_needs_page(tables[2], idx[2], user);
				tables[3] = tables[2]->get(idx[2]);

				for (; idx[3] < table_entries; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys_addr | flags;
					phys_addr += frame_size;
					if (--count == 0) return;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_in");
}

void
page_manager::page_out(page_table *pml4, uintptr_t virt_addr, size_t count, bool free)
{
	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	uintptr_t free_start = 0;
	unsigned free_count = 0;

	for (; idx[0] < table_entries; idx[0] += 1) {
		page_table *table = tables[0]->get(idx[0]);
		if (!table) {
			constexpr size_t skip = 512 * 512 * 512;
			if (count > skip) {
				count -= skip;
				continue;
			}
			goto page_out_end;
		}

		tables[1] = table;

		for (; idx[1] < table_entries; idx[1] += 1) {
			page_table *table = tables[1]->get(idx[1]);
			if (!table) {
				constexpr size_t skip = 512 * 512;
				if (count > skip) {
					count -= skip;
					continue;
				}
				goto page_out_end;
			}

			tables[2] = table;

			for (; idx[2] < table_entries; idx[2] += 1) {
				page_table *table = tables[2]->get(idx[2]);
				if (!table) {
					constexpr size_t skip = 512;
					if (count > skip) {
						count -= skip;
						continue;
					}
					goto page_out_end;
				}

				tables[3] = table;

				for (; idx[3] < table_entries; idx[3] += 1) {
					uintptr_t entry = tables[3]->entries[idx[3]];
					bool present = entry & 1;

					if (present) {
						entry &= ~0xfffull;
						if (!free_count || entry != free_start + free_count * frame_size) {
							if (free_count && free) m_frames.free(free_start, free_count);
							free_start = tables[3]->entries[idx[3]] & ~0xfffull;
							free_count = 1;
						} else {
							free_count++;
						}

						tables[3]->entries[idx[3]] = 0;
					}

					if (--count == 0)
						goto page_out_end;
				}
			}
		}
	}

page_out_end:
	if (free && free_count)
		m_frames.free(free_start, free_count);
}
