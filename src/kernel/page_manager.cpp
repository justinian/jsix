#include "kutil/assert.h"
#include "kutil/vm_space.h"
#include "console.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"

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


struct free_page_header
{
	free_page_header *next;
	size_t count;
};


page_manager::page_manager(frame_allocator &frames, page_table *pml4) :
	m_kernel_pml4(pml4),
	m_page_cache(nullptr),
	m_frames(frames)
{
}

page_table *
page_manager::create_process_map()
{
	page_table *table = get_table_page();

	kutil::memset(table, 0, frame_size/2);
	for (unsigned i = pml4e_kernel; i < table_entries; ++i)
		table->entries[i] = m_kernel_pml4->entries[i];

	return table;
}

uintptr_t
page_manager::copy_page(uintptr_t orig)
{
	uintptr_t copy = 0;
	size_t n = m_frames.allocate(1, &copy);
	kassert(n, "copy_page could not allocate page");

	uintptr_t orig_virt = orig + page_offset;
	uintptr_t copy_virt = copy + page_offset;

	kutil::memcpy(
			reinterpret_cast<void *>(copy_virt),
			reinterpret_cast<void *>(orig_virt),
			frame_size);

	return copy;
}

page_table *
page_manager::copy_table(page_table *from, page_table::level lvl, page_table_indices index)
{
	page_table *to = get_table_page();
	log::debug(logs::paging, "Page manager copying level %d table at %016lx to %016lx.", lvl, from, to);

	if (lvl == page_table::level::pml4) {
		for (unsigned i = pml4e_kernel; i < table_entries; ++i)
			to->entries[i] = m_kernel_pml4->entries[i];
	}

	const int max =
		lvl == page_table::level::pml4
			? pml4e_kernel
			: table_entries;

	unsigned pages_copied = 0;
	uintptr_t from_addr = 0;
	uintptr_t to_addr = 0;

	for (int i = 0; i < max; ++i) {
		if (!from->is_present(i)) {
			to->entries[i] = 0;
			continue;
		}

		index[lvl] = i;

		bool is_page =
			lvl == page_table::level::pt ||
			from->is_large_page(lvl, i);

		if (is_page) {
			uint16_t flags = from->entries[i] &  0xfffull;
			uintptr_t orig = from->entries[i] & ~0xfffull;
			to->entries[i] = copy_page(orig) | flags;
			if (!pages_copied++)
				from_addr = index.addr();
			to_addr = index.addr();
		} else {
			uint16_t flags = 0;
			page_table *next_from = from->get(i, &flags);
			page_table *next_to = copy_table(next_from, page_table::deeper(lvl), index);
			to->set(i, next_to, flags);
		}
	}

	if (pages_copied)
		log::debug(logs::paging, "   copied %3u pages %016lx - %016lx",
			pages_copied, from_addr, to_addr + frame_size);

	return to;
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

void
page_manager::map_offset_pointer(void **pointer, size_t length)
{
	log::debug(logs::paging, "Mapping offset pointer region at %016lx size 0x%lx", *pointer, length);
	*pointer = kutil::offset_pointer(*pointer, page_offset);
}

void
page_manager::dump_pml4(page_table *pml4, bool recurse)
{
	if (pml4 == nullptr) pml4 = get_pml4();
	pml4->dump(page_table::level::pml4, recurse);
}

page_table *
page_manager::get_table_page()
{
	if (!m_page_cache) {
		uintptr_t phys = 0;
		size_t n = m_frames.allocate(32, &phys); // TODO: indicate frames must be offset-mappable
		uintptr_t virt = phys + page_offset;

		m_page_cache = reinterpret_cast<free_page_header *>(virt);

		// The last one needs to be null, so do n-1
		uintptr_t end = virt + (n-1) * frame_size;
		while (virt < end) {
			reinterpret_cast<free_page_header *>(virt)->next =
				reinterpret_cast<free_page_header *>(virt + frame_size);
			virt += frame_size;
		}
		reinterpret_cast<free_page_header *>(virt)->next = nullptr;

		log::info(logs::paging, "Mappd %d new page table pages at %lx", n, phys);
	}

	free_page_header *page = m_page_cache;
	m_page_cache = page->next;

	return reinterpret_cast<page_table *>(page);
}

void
page_manager::free_table_pages(void *pages, size_t count)
{
	uintptr_t start = reinterpret_cast<uintptr_t>(pages);
	for (size_t i = 0; i < count; ++i) {
		uintptr_t addr = start + (i * frame_size);
		free_page_header *header = reinterpret_cast<free_page_header *>(addr);
		header->count = 1;
		header->next = m_page_cache;
		m_page_cache = header;
	}
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
	free_table_pages(table, 1);

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

bool
page_manager::fault_handler(uintptr_t addr)
{
	if (!addr)
		return false;

	extern kutil::vm_space g_kernel_space;
	bool is_heap = addr >= ::memory::heap_start &&
		addr < ::memory::heap_start + ::memory::kernel_max_heap;

	if (!is_heap &&
		g_kernel_space.get(addr) != kutil::vm_state::committed)
		return false;

	uintptr_t page = addr & ~0xfffull;
	log::debug(logs::memory, "PF: attempting to page in %016lx for %016lx", page, addr);

	bool user = addr < kernel_offset;
	map_pages(page, 1, user);

	// Kernel stacks: zero them upon mapping them
	if (addr >= memory::stacks_start && addr < memory::heap_start)
		kutil::memset(reinterpret_cast<void*>(page), 0, memory::frame_size);

	return true;
}

void
page_manager::check_needs_page(page_table *table, unsigned index, bool user)
{
	if ((table->entries[index] & 0x1) == 1) return;

	page_table *new_table = get_table_page();
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

void
page_table::dump(page_table::level lvl, bool recurse)
{
	console *cons = console::get();

	cons->printf("\nLevel %d page table @ %lx:\n", lvl, this);
	for (int i=0; i<table_entries; ++i) {
		uint64_t ent = entries[i];

		if ((ent & 0x1) == 0)
			cons->printf("  %3d: %016lx   NOT PRESENT\n", i, ent);

		else if ((lvl == level::pdp || lvl == level::pd) && (ent & 0x80) == 0x80)
			cons->printf("  %3d: %016lx -> Large page at    %016lx\n", i, ent, ent & ~0xfffull);

		else if (lvl == level::pt)
			cons->printf("  %3d: %016lx -> Page at          %016lx\n", i, ent, ent & ~0xfffull);

		else
			cons->printf("  %3d: %016lx -> Level %d table at %016lx\n",
					i, ent, deeper(lvl), (ent & ~0xfffull) + page_offset);
	}

	if (lvl != level::pt && recurse) {
		for (int i=0; i<=table_entries; ++i) {
			if (is_large_page(lvl, i))
				continue;

			page_table *next = get(i);
			if (next)
				next->dump(deeper(lvl), true);
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
