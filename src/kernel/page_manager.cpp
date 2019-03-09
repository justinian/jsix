#include <algorithm>

#include "kutil/assert.h"
#include "console.h"
#include "log.h"
#include "page_manager.h"

using memory::frame_size;
using memory::kernel_offset;
using memory::page_offset;

extern kutil::frame_allocator g_frame_allocator;
extern kutil::address_manager g_kernel_address_manager;
page_manager g_page_manager(
	g_frame_allocator,
	g_kernel_address_manager);


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


page_manager::page_manager(
		kutil::frame_allocator &frames,
		kutil::address_manager &addrs) :
	m_page_cache(nullptr),
	m_frames(frames),
	m_addrs(addrs)
{
}

page_table *
page_manager::create_process_map()
{
	page_table *table = get_table_page();

	kutil::memset(table, 0, frame_size);
	table->entries[510] = m_kernel_pml4->entries[510];
	table->entries[511] = m_kernel_pml4->entries[511];

	// Create the initial user stack
	map_pages(
		memory::initial_stack - (memory::initial_stack_pages * frame_size),
		memory::initial_stack_pages,
		true, // This is the ring3 stack, user = true
		table);

	return table;
}

uintptr_t
page_manager::copy_page(uintptr_t orig)
{
	uintptr_t virt = m_addrs.allocate(2 * frame_size);
	uintptr_t copy = 0;

	size_t n = m_frames.allocate(1, &copy);
	kassert(n, "copy_page could not allocate page");

	page_in(get_pml4(), orig, virt, 1);
	page_in(get_pml4(), copy, virt + frame_size, 1);

	kutil::memcpy(
			reinterpret_cast<void *>(virt + frame_size),
			reinterpret_cast<void *>(virt),
			frame_size);

	page_out(get_pml4(), virt, 2);

	m_addrs.free(virt);
	return copy;
}

page_table *
page_manager::copy_table(page_table *from, page_table::level lvl)
{
	page_table *to = get_table_page();
	log::debug(logs::paging, "Page manager copying level %d table at %016lx to %016lx.", lvl, from, to);

	if (lvl == page_table::level::pml4) {
		to->entries[510] = m_kernel_pml4->entries[510];
		to->entries[511] = m_kernel_pml4->entries[511];
	}

	const int max =
		lvl == page_table::level::pml4 ?
			510 :
			512;

	unsigned pages_copied = 0;
	for (int i = 0; i < max; ++i) {
		if (!from->is_present(i)) {
			to->entries[i] = 0;
			continue;
		}

		bool is_page =
			lvl == page_table::level::pt ||
			from->is_large_page(lvl, i);

		if (is_page) {
			uint16_t flags = from->entries[i] &  0xfffull;
			uintptr_t orig = from->entries[i] & ~0xfffull;
			to->entries[i] = copy_page(orig) | flags;
			pages_copied++;
		} else {
			uint16_t flags = 0;
			page_table *next_from = from->get(i, &flags);
			page_table *next_to = copy_table(next_from, page_table::deeper(lvl));
			to->set(i, next_to, flags);
		}
	}

	if (pages_copied)
		log::debug(logs::paging, "   copied %3u pages", pages_copied);

	return to;
}

void
page_manager::delete_process_map(page_table *pml4)
{
	unmap_table(pml4, page_table::level::pml4, true);
}

void
page_manager::map_offset_pointer(void **pointer, size_t length)
{
	log::info(logs::paging, "Mapping offset pointer region at %016lx size 0x%lx", *pointer, length);
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
page_manager::unmap_table(page_table *table, page_table::level lvl, bool free)
{
	const int max =
		lvl == page_table::level::pml4 ?
			510 :
			512;

	uintptr_t free_start = 0;
	uintptr_t free_count = 0;

	size_t size =
		lvl == page_table::level::pdp ? (1<<30) :
		lvl == page_table::level::pd  ? (1<<21) :
		lvl == page_table::level::pt  ? (1<<12) :
		0;

	for (int i = 0; i < max; ++i) {
		if (!table->is_present(i)) continue;

		bool is_page =
			lvl == page_table::level::pt ||
			table->is_large_page(lvl, i);

		if (is_page) {
			uintptr_t frame = table->entries[i] & ~0xfffull;
			if (!free_count || free_start != frame + free_count * size) {
				if (free_count && free)
					m_frames.free(free_start, free_count * size / frame_size);
				free_start = frame;
				free_count = 1;
			}
		} else {
			page_table *next = table->get(i);
			unmap_table(next, page_table::deeper(lvl), free);
		}
	}

	if (free_count && free)
		m_frames.free(free_start, free_count * size / frame_size);
	free_table_pages(table, 1);
}

void
page_manager::unmap_pages(void* address, size_t count, page_table *pml4)
{
	if (!pml4) pml4 = get_pml4();
	page_out(pml4, reinterpret_cast<uintptr_t>(address), count, true);
}

void
page_manager::check_needs_page(page_table *table, unsigned index, bool user)
{
	if ((table->entries[index] & 0x1) == 1) return;

	page_table *new_table = get_table_page();
	for (int i=0; i<512; ++i) new_table->entries[i] = 0;
	table->entries[index] = pt_to_phys(new_table) | (user ? 0xf : 0xb);
}

void
page_manager::page_in(page_table *pml4, uintptr_t phys_addr, uintptr_t virt_addr, size_t count, bool user, bool large)
{
	log::debug(logs::paging, "page_in for table %016lx p:%016lx v:%016lx c:%4d u:%d l:%d",
			pml4, phys_addr, virt_addr, count, user, large);

	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	uint64_t flags = user ?
		0x00f:	// writethru, user, write, present
		0x10b;	// global, writethru, write, present

	for (; idx[0] < 512; idx[0] += 1) {
		check_needs_page(tables[0], idx[0], user);
		tables[1] = tables[0]->get(idx[0]);

		for (; idx[1] < 512; idx[1] += 1, idx[2] = 0, idx[3] = 0) {
			check_needs_page(tables[1], idx[1], user);
			tables[2] = tables[1]->get(idx[1]);

			for (; idx[2] < 512; idx[2] += 1, idx[3] = 0) {
				if (large &&
					idx[3] == 0 &&
					count >= 512 &&
					tables[2]->get(idx[2]) == nullptr) {
					// Do a 2MiB page instead
					tables[2]->entries[idx[2]] = phys_addr | flags | 0x80;
					phys_addr += frame_size * 512;
					count -= 512;
					if (count == 0) return;
					continue;
				}

				check_needs_page(tables[2], idx[2], user);
				tables[3] = tables[2]->get(idx[2]);

				for (; idx[3] < 512; idx[3] += 1) {
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

	bool found = false;
	uintptr_t free_start = 0;
	unsigned free_count = 0;

	for (; idx[0] < 512; idx[0] += 1) {
		tables[1] = tables[0]->get(idx[0]);

		for (; idx[1] < 512; idx[1] += 1) {
			tables[2] = tables[1]->get(idx[1]);

			for (; idx[2] < 512; idx[2] += 1) {
				tables[3] = tables[2]->get(idx[2]);

				for (; idx[3] < 512; idx[3] += 1) {
					uintptr_t entry = tables[3]->entries[idx[3]] & ~0xfffull;
					if (!found || entry != free_start + free_count * frame_size) {
						if (found && free) m_frames.free(free_start, free_count);
						free_start = tables[3]->entries[idx[3]] & ~0xfffull;
						free_count = 1;
						found = true;
					} else {
						free_count++;
					}

					tables[3]->entries[idx[3]] = 0;

					if (--count == 0) {
						if (free) m_frames.free(free_start, free_count);
						return;
					}
				}
			}
		}
	}

	kassert(0, "Ran to end of page_out");
}

void
page_table::dump(page_table::level lvl, bool recurse)
{
	console *cons = console::get();

	cons->printf("\nLevel %d page table @ %lx:\n", lvl, this);
	for (int i=0; i<512; ++i) {
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
		for (int i=0; i<=512; ++i) {
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
