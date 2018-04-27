#include <algorithm>

#include "assert.h"
#include "console.h"
#include "memory_pages.h"

page_manager g_page_manager;

using addr_t = page_manager::addr_t;


static addr_t
pt_to_phys(page_table *pt)
{
	return reinterpret_cast<addr_t>(pt) - page_manager::page_offset;
}

static page_table *
pt_from_phys(addr_t p)
{
	return reinterpret_cast<page_table *>((p + page_manager::page_offset) & ~0xfffull);
}


struct free_page_header
{
	free_page_header *next;
	size_t count;
};

size_t
page_block::length(page_block *list)
{
	size_t i = 0;
	for (page_block *b = list; b; b = b->next) ++i;
	return i;
}

page_block *
page_block::append(page_block *list, page_block *extra)
{
	if (list == nullptr) return extra;
	else if (extra == nullptr) return list;

	page_block *cur = list;
	while (cur->next)
		cur = cur->next;
	cur->next = extra;
	return list;
}

page_block *
page_block::insert(page_block *list, page_block *block)
{
	if (list == nullptr) return block;
	else if (block == nullptr) return list;

	page_block *cur = list;
	page_block *prev = nullptr;
	while (cur && page_block::compare(block, cur) > 0) {
		prev = cur;
		cur = cur->next;
	}

	block->next = cur;
	if (prev) {
		prev->next = block;
		return list;
	}
	return block;
}

int
page_block::compare(const page_block *lhs, const page_block *rhs)
{
	if (lhs->virtual_address < rhs->virtual_address)
		return -1;
	else if (lhs->virtual_address > rhs->virtual_address)
		return 1;

	if (lhs->physical_address < rhs->physical_address)
		return -1;
	else if (lhs->physical_address > rhs->physical_address)
		return 1;

	return 0;
}

page_block *
page_block::consolidate(page_block *list)
{
	page_block *freed = nullptr;
	page_block *cur = list;

	while (cur) {
		page_block *next = cur->next;

		if (next &&
			cur->flags == next->flags &&
			cur->physical_end() == next->physical_address &&
			(!cur->has_flag(page_block_flags::mapped) ||
			cur->virtual_end() == next->virtual_address)) {

			cur->count += next->count;
			cur->next = next->next;

			next->zero(freed);
			freed = next;
			continue;
		}

		cur = cur->next;
	}

	return freed;
}

void
page_block::dump(page_block *list, const char *name, bool show_unmapped)
{
	console *cons = console::get();
	cons->printf("Block list %s:\n", name);

	int count = 0;
	for (page_block *cur = list; cur; cur = cur->next) {
		count += 1;
		if (!(show_unmapped || cur->has_flag(page_block_flags::mapped)))
			continue;

		cons->printf("  %lx %x [%6d]",
				cur->physical_address,
				cur->flags,
				cur->count);

		if (cur->virtual_address) {
			page_table_indices start{cur->virtual_address};
			page_table_indices end{cur->virtual_address + cur->count * page_manager::page_size - 1};
			cons->printf(" %lx (%d,%d,%d,%d)-(%d,%d,%d,%d)",
					cur->virtual_address,
					start[0], start[1], start[2], start[3],
					end[0], end[1], end[2], end[3]);
		}

		cons->printf("\n");

	}

	cons->printf("  Total: %d\n", count);
}

void
page_block::zero(page_block *set_next)
{
	physical_address = 0;
	virtual_address = 0;
	count = 0;
	flags = page_block_flags::free;
	next = set_next;
}

void
page_block::copy(page_block *other)
{
	physical_address = other->physical_address;
	virtual_address = other->virtual_address;
	count = other->count;
	flags = other->flags;
	next = other->next;
}


page_manager::page_manager() :
	m_free(nullptr),
	m_used(nullptr),
	m_block_cache(nullptr),
	m_page_cache(nullptr)
{
	kassert(this == &g_page_manager, "Attempt to create another page_manager.");
}

void
page_manager::init(
	page_block *free,
	page_block *used,
	page_block *block_cache)
{
	m_free = free;
	m_used = used;
	m_block_cache = block_cache;

	// For now we're ignoring that we've got the scratch pages
	// allocated, full of page_block structs. Eventually hand
	// control of that to a slab allocator.

	page_table *pml4 = get_pml4();

	// Fix up the offset-marked pointers
	for (unsigned i = 0; i < m_marked_pointer_count; ++i) {
		addr_t *p = reinterpret_cast<addr_t *>(m_marked_pointers[i]);
		addr_t v = *p + page_offset;
		addr_t c = (m_marked_pointer_lengths[i] / page_size) + 1;

		// TODO: cleanly search/split this as a block out of used/free if possible
		page_block *block = get_block();

		// TODO: page-align
		block->physical_address = *p;
		block->virtual_address = v;
		block->count = c;
		block->flags =
			page_block_flags::used |
			page_block_flags::mapped |
			page_block_flags::mmio;

		console::get()->printf("Fixing up pointer %lx [%3d] -> %lx\n", *p, c, v);

		m_used = page_block::insert(m_used, block);
		page_in(pml4, *p, v, c);
		*p = v;
	}

	consolidate_blocks();

	page_block::dump(m_used, "used", true);
	page_block::dump(m_free, "free", true);

}

void
page_manager::mark_offset_pointer(void **pointer, size_t length)
{
	m_marked_pointers[m_marked_pointer_count] = pointer;
	m_marked_pointer_lengths[m_marked_pointer_count++] = length;
}

page_block *
page_manager::get_block()
{
	page_block *block = m_block_cache;
	if (block) {
		m_block_cache = block->next;
		block->next = 0;
		return block;
	} else {
		kassert(0, "NYI: page_manager::get_block() needed to allocate.");
	}
}

void
page_manager::free_blocks(page_block *block)
{
	if (!block) return;

	page_block *cur = block;
	while (cur) {
		page_block *next = cur->next;
		cur->zero(cur->next ? cur->next : m_block_cache);
		cur = next;
	}

	m_block_cache = block;
}

page_table *
page_manager::get_table_page()
{
	if (!m_page_cache) {
		addr_t phys = 0;
		size_t n = pop_pages(32, &phys);
		addr_t virt = phys + page_offset;

		page_block *block = get_block();
		block->physical_address = phys;
		block->virtual_address = virt;
		block->count = n;
		page_block::insert(m_used, block);

		page_in(get_pml4(), phys, virt, n);

		m_page_cache = reinterpret_cast<free_page_header *>(virt);

		// The last one needs to be null, so do n-1
		addr_t end = virt + (n-1) * page_size;
		while (virt < end) {
			reinterpret_cast<free_page_header *>(virt)->next =
				reinterpret_cast<free_page_header *>(virt + page_size);
			virt += page_size;
		}
		reinterpret_cast<free_page_header *>(virt)->next = nullptr;

		g_console.printf("Mappd %d new page table pages at %lx\n", n, phys);
	}

	free_page_header *page = m_page_cache;
	m_page_cache = page->next;
	return reinterpret_cast<page_table *>(page);
}

void
page_manager::free_table_pages(void *pages, size_t count)
{
	addr_t start = reinterpret_cast<addr_t>(pages);
	for (size_t i = 0; i < count; ++i) {
		addr_t addr = start + (i * page_size);
		free_page_header *header = reinterpret_cast<free_page_header *>(addr);
		header->count = 1;
		header->next = m_page_cache;
		m_page_cache = header;
	}
}

void
page_manager::consolidate_blocks()
{
	m_block_cache = page_block::append(m_block_cache, page_block::consolidate(m_free));
	m_block_cache = page_block::append(m_block_cache, page_block::consolidate(m_used));
}

void *
page_manager::map_pages(addr_t address, size_t count)
{
	void *ret = reinterpret_cast<void *>(address);
	page_table *pml4 = get_pml4();

	while (count) {
		kassert(m_free, "page_manager::map_pages ran out of free pages!");

		addr_t phys = 0;
		size_t n = pop_pages(count, &phys);

		page_block *block = get_block();
		block->physical_address = phys;
		block->virtual_address = address;
		block->count = n;
		page_block::insert(m_used, block);

		page_in(pml4, phys, address, n);

		address += n * page_size;
		count -= n;
	}

	return ret;
}

void
page_manager::unmap_pages(addr_t address, size_t count)
{
	page_block **prev = &m_used;
	page_block *cur = m_used;
	while (cur && !cur->contains(address)) {
		prev = &cur->next;
		cur = cur->next;
	}

	kassert(cur, "Couldn't find existing mapped pages to unmap");

	size_t size = page_size * count;
	addr_t end = address + size;

	while (cur && cur->contains(address)) {
		size_t leading = address - cur->virtual_address;
		size_t trailing =
			end > cur->virtual_end() ?
			0 : (cur->virtual_end() - end);

		if (leading) {
			size_t pages = leading / page_size;

			page_block *lead_block = get_block();
			lead_block->copy(cur);
			lead_block->next = cur;
			lead_block->count = pages;

			cur->count -= pages;
			cur->physical_address += leading;
			cur->virtual_address += leading;

			*prev = lead_block;
			prev = &lead_block->next;
		}

		if (trailing) {
			size_t pages = trailing / page_size;

			page_block *trail_block = get_block();
			trail_block->copy(cur);
			trail_block->next = cur->next;
			trail_block->count = pages;
			trail_block->physical_address += size;
			trail_block->virtual_address += size;

			cur->count -= pages;

			cur->next = trail_block;
		}

		address += cur->count * page_size;
		page_block *next = cur->next;

		*prev = cur->next;
		cur->next = nullptr;
		cur->virtual_address = 0;
		cur->flags = cur->flags & ~(page_block_flags::used | page_block_flags::mapped);
		m_free = page_block::insert(m_free, cur);

		cur = next;
	}
}

void
page_manager::check_needs_page(page_table *table, unsigned index)
{
	if (table->entries[index] & 0x1 == 1) return;

	page_table *new_table = get_table_page();
	for (int i=0; i<512; ++i) new_table->entries[i] = 0;
	table->entries[index] = pt_to_phys(new_table) | 0xb;
}

void
page_manager::page_in(page_table *pml4, addr_t phys_addr, addr_t virt_addr, size_t count)
{
	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	for (; idx[0] < 512; idx[0] += 1) {
		check_needs_page(tables[0], idx[0]);
		tables[1] = tables[0]->get(idx[0]);

		for (; idx[1] < 512; idx[1] += 1, idx[2] = 0, idx[3] = 0) {
			check_needs_page(tables[1], idx[1]);
			tables[2] = tables[1]->get(idx[1]);

			for (; idx[2] < 512; idx[2] += 1, idx[3] = 0) {
				check_needs_page(tables[2], idx[2]);
				tables[3] = tables[2]->get(idx[2]);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys_addr | 0xb;
					phys_addr += page_manager::page_size;
					if (--count == 0) return;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_in");
}

void
page_manager::page_out(page_table *pml4, addr_t virt_addr, size_t count)
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

size_t
page_manager::pop_pages(size_t count, addr_t *address)
{
	kassert(m_free, "page_manager::pop_pages ran out of free pages!");

	unsigned n = std::min(count, static_cast<size_t>(m_free->count));
	*address = m_free->physical_address;

	m_free->physical_address += n * page_size;
	m_free->count -= n;
	if (m_free->count == 0) {
		page_block *block = m_free;
		m_free = m_free->next;

		block->zero(m_block_cache);
		m_block_cache = block;
	}

	return n;
}


void
page_table::dump(int level, uint64_t offset)
{
	console *cons = console::get();
	cons->printf("Level %d page table @ %lx (off %lx):\n", level, this, offset);
	for (int i=0; i<512; ++i) {
		uint64_t ent = entries[i];
		if (ent == 0) continue;

		cons->printf("  %3d: %lx ", i, ent);
		if (ent & 0x1 == 0) {
			cons->printf("  NOT PRESENT\n");
			continue;
		}

		if ((level == 2 || level == 3) && (ent & 0x80) == 0x80) {
			cons->printf("  -> Large page at %lx\n", ent & ~0xfffull);
			continue;
		} else if (level == 1) {
			cons->printf("  -> Page at %lx\n", (ent & ~0xfffull));
		} else {
			cons->printf("  -> Level %d table at %lx\n", level - 1, (ent & ~0xfffull) + offset);
			continue;
		}
	}
	cons->printf("\n");

	if (--level > 0) {
		for (int i=0; i<512; ++i) {
			uint64_t ent = entries[i];
			if ((ent & 0x1) == 0) continue;
			if ((ent & 0x80)) continue;

			page_table *next = reinterpret_cast<page_table *>((ent & ~0xffful) + offset); 
			next->dump(level, offset);
		}
	}
}


