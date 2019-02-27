#include <algorithm>

#include "kutil/assert.h"
#include "console.h"
#include "log.h"
#include "page_manager.h"

kutil::frame_allocator g_frame_allocator;
page_manager g_page_manager(g_frame_allocator);


static uintptr_t
pt_to_phys(page_table *pt)
{
	return reinterpret_cast<uintptr_t>(pt) - page_manager::page_offset;
}


static page_table *
pt_from_phys(uintptr_t p)
{
	return reinterpret_cast<page_table *>((p + page_manager::page_offset) & ~0xfffull);
}


struct free_page_header
{
	free_page_header *next;
	size_t count;
};


page_manager::page_manager(kutil::frame_allocator &frames) :
	m_page_cache(nullptr),
	m_frames(frames)
{
}

page_table *
page_manager::create_process_map()
{
	page_table *table = get_table_page();

	kutil::memset(table, 0, page_size);
	table->entries[510] = m_kernel_pml4->entries[510];
	table->entries[511] = m_kernel_pml4->entries[511];

	// Create the initial user stack
	map_pages(
		initial_stack - (initial_stack_pages * page_size),
		initial_stack_pages,
		true, // This is the ring3 stack, user = true
		table);

	return table;
}

void
page_manager::delete_process_map(page_table *table)
{
	// TODO: recurse table entries and mark them free
	unmap_pages(table, 1);
}

void
page_manager::map_offset_pointer(void **pointer, size_t length)
{
	uintptr_t *p = reinterpret_cast<uintptr_t *>(pointer);
	uintptr_t v = *p + page_offset;
	uintptr_t c = ((length - 1) / page_size) + 1;

	page_table *pml4 = get_pml4();
	page_in(pml4, *p, v, c);
	*p = v;
}

void
page_manager::dump_pml4(page_table *pml4, int max_index)
{
	if (pml4 == nullptr)
		pml4 = get_pml4();
	pml4->dump(4, max_index);
}

page_table *
page_manager::get_table_page()
{
	if (!m_page_cache) {
		uintptr_t phys = 0;
		size_t n = m_frames.allocate(32, &phys);
		uintptr_t virt = phys + page_offset;
		page_in(get_pml4(), phys, virt, n);

		m_page_cache = reinterpret_cast<free_page_header *>(virt);

		// The last one needs to be null, so do n-1
		uintptr_t end = virt + (n-1) * page_size;
		while (virt < end) {
			reinterpret_cast<free_page_header *>(virt)->next =
				reinterpret_cast<free_page_header *>(virt + page_size);
			virt += page_size;
		}
		reinterpret_cast<free_page_header *>(virt)->next = nullptr;

		log::debug(logs::memory, "Mappd %d new page table pages at %lx", n, phys);
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
		uintptr_t addr = start + (i * page_size);
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

		log::debug(logs::memory, "Paging in %d pages at p:%016lx to v:%016lx into %016lx table",
				n, phys, address, pml4);

		page_in(pml4, phys, address, n, user);

		address += n * page_size;
		count -= n;
	}

	return ret;
}

void
page_manager::unmap_pages(void* address, size_t count)
{
	// TODO: uh, actually unmap that shit??
	m_frames.free(reinterpret_cast<uintptr_t>(address), count);
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
page_manager::page_in(page_table *pml4, uintptr_t phys_addr, uintptr_t virt_addr, size_t count, bool user)
{
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
				if (idx[3] == 0 &&
					count >= 512 &&
					tables[2]->get(idx[2]) == nullptr) {
					// Do a 2MiB page instead
					tables[2]->entries[idx[2]] = phys_addr | flags | 0x80;
					phys_addr += page_size * 512;
					count -= 512;
					if (count == 0) return;
					continue;
				}

				check_needs_page(tables[2], idx[2], user);
				tables[3] = tables[2]->get(idx[2]);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys_addr | flags;
					phys_addr += page_size;
					if (--count == 0) return;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_in");
}

void
page_manager::page_out(page_table *pml4, uintptr_t virt_addr, size_t count)
{
	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	for (; idx[0] < 512; idx[0] += 1) {
		tables[1] = reinterpret_cast<page_table *>(
				tables[0]->entries[idx[0]] & ~0xfffull);

		for (; idx[1] < 512; idx[1] += 1) {
			tables[2] = reinterpret_cast<page_table *>(
					tables[1]->entries[idx[1]] & ~0xfffull);

			for (; idx[2] < 512; idx[2] += 1) {
				tables[3] = reinterpret_cast<page_table *>(
						tables[2]->entries[idx[2]] & ~0xfffull);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = 0;
					if (--count == 0) return;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_out");
}

void
page_table::dump(int level, int max_index, uint64_t offset)
{
	console *cons = console::get();

	cons->printf("\nLevel %d page table @ %lx (off %lx):\n", level, this, offset);
	for (int i=0; i<512; ++i) {
		uint64_t ent = entries[i];
		if (ent == 0) continue;

		if ((ent & 0x1) == 0) {
			cons->printf("  %3d: %016lx   NOT PRESENT\n", i, ent);
			continue;
		}

		if ((level == 2 || level == 3) && (ent & 0x80) == 0x80) {
			cons->printf("  %3d: %016lx -> Large page at    %016lx\n", i, ent, ent & ~0xfffull);
			continue;
		} else if (level == 1) {
			cons->printf("  %3d: %016lx -> Page at          %016lx\n", i, ent, ent & ~0xfffull);
		} else {
			cons->printf("  %3d: %016lx -> Level %d table at %016lx\n",
					i, ent, level - 1, (ent & ~0xfffull) + offset);
			continue;
		}
	}

	if (--level > 0) {
		for (int i=0; i<=max_index; ++i) {
			uint64_t ent = entries[i];
			if ((ent & 0x1) == 0) continue;
			if ((ent & 0x80)) continue;

			page_table *next = reinterpret_cast<page_table *>((ent & ~0xffful) + offset); 
			next->dump(level, 511, offset);
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
