#include "assert.h"
#include "console.h"
#include "memory_pages.h"

page_manager g_page_manager;


struct free_page_header
{
	free_page_header *next;
	size_t count;
};

size_t
page_block::list_count()
{
	size_t i = 0;
	for (page_block *b = this; b; b = b->next) ++i;
	return i;
}

void
page_block::list_append(page_block *list)
{
	page_block *cur = this;
	while (cur->next) cur = cur->next;
	cur->next = list;
}

page_block *
page_block::list_insert(page_block *block)
{
	page_block *cur = this;
	page_block **prev = nullptr;
	while (cur->physical_address < block->physical_address) {
		prev = &cur->next;
		cur = cur->next;
	}

	block->next = cur;
	if (prev) {
		*prev = block;
		return this;
	}
	return block;
}

page_block *
page_block::list_consolidate()
{
	page_block *freed = nullptr;
	page_block *cur = this;

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
page_block::list_dump(const char *name, bool show_unmapped)
{
	console *cons = console::get();
	cons->puts("Block list");
	if (name) {
		cons->puts(" ");
		cons->puts(name);
	}
	cons->puts(":\n");

	int count = 0;
	for (page_block *cur = this; cur; cur = cur->next) {
		count += 1;
		if (!(show_unmapped || cur->has_flag(page_block_flags::mapped)))
			continue;

		cons->puts("  ");
		cons->put_hex(cur->physical_address);
		cons->puts(" ");
		cons->put_hex((uint32_t)cur->flags);
		if (cur->virtual_address) {
			cons->puts(" ");
			cons->put_hex(cur->virtual_address);
		}
		cons->puts(" [");
		cons->put_dec(cur->count);
		cons->puts("]\n");

	}

	cons->puts("  Total: ");
	cons->put_dec(count);
	cons->puts("\n");
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
	page_block *block_cache,
	uint64_t scratch_start,
	uint64_t scratch_pages,
	uint64_t scratch_cur)
{
	m_free = free;
	m_used = used;
	m_block_cache = block_cache;

	kassert(scratch_cur == page_align(scratch_cur),
			"Current scratch space pointer is not page-aligned.");

	uint64_t scratch_end = scratch_start + page_size * scratch_pages;
	uint64_t unused_pages = (scratch_end - scratch_cur) / page_size;

	console *cons = console::get();

	unmap_pages(scratch_cur, unused_pages);
	consolidate_blocks();

	uint64_t scratch_aligned_start = page_table_align(scratch_start);
	if (scratch_aligned_start != scratch_start) {
		free_page_header *header =
			reinterpret_cast<free_page_header *>(scratch_start);
		header->count = (scratch_aligned_start - scratch_start) / page_size;
		header->next = m_page_cache;
		m_page_cache = header;
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

void *
page_manager::map_pages(uint64_t address, unsigned count)
{
}

void
page_manager::unmap_pages(uint64_t address, unsigned count)
{
	page_block **prev = &m_used;
	page_block *cur = m_used;
	while (cur && !cur->contains(address)) {
		prev = &cur->next;
		cur = cur->next;
	}

	kassert(cur, "Couldn't find existing mapped pages to unmap");

	uint64_t leading = address - cur->virtual_address;
	uint64_t trailing = cur->virtual_end() - (address + page_size*count);

	if (leading) {
		page_block *lead_block = get_block();
		lead_block->copy(cur);
		lead_block->next = cur;
		lead_block->count = leading / page_size;
		*prev = lead_block;
		prev = &lead_block->next;
	}

	if (trailing) {
		page_block *trail_block = get_block();
		trail_block->copy(cur);
		trail_block->next = cur->next;
		trail_block->count = trailing / page_size;
		cur->next = trail_block;
	}

	*prev = cur->next;
	cur->next = nullptr;
	cur->flags = cur->flags & ~(page_block_flags::used | page_block_flags::mapped);
	m_free->list_insert(cur);
}

void
page_manager::consolidate_blocks()
{
	m_block_cache->list_append(m_free->list_consolidate());
	m_block_cache->list_append(m_used->list_consolidate());
}

static unsigned
check_needs_page(page_table *table, unsigned index, page_table **free_pages)
{
	if (table->entries[index] & 0x1 == 1) return 0;

	kassert(*free_pages, "check_needs_page needed to allocate but had no free pages");

	page_table *new_table = (*free_pages)++;
	for (int i=0; i<512; ++i) new_table->entries[i] = 0;
	table->entries[index] = reinterpret_cast<uint64_t>(new_table) | 0xb;
	return 1;
}

unsigned
page_in(page_table *pml4, uint64_t phys_addr, uint64_t virt_addr, uint64_t count, page_table *free_pages)
{
	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	unsigned pages_consumed = 0;
	for (; idx[0] < 512; idx[0] += 1) {
		pages_consumed += check_needs_page(tables[0], idx[0], &free_pages);
		tables[1] = reinterpret_cast<page_table *>(
				tables[0]->entries[idx[0]] & ~0xfffull);

		for (; idx[1] < 512; idx[1] += 1, idx[2] = 0, idx[3] = 0) {
			pages_consumed += check_needs_page(tables[1], idx[1], &free_pages);
			tables[2] = reinterpret_cast<page_table *>(
					tables[1]->entries[idx[1]] & ~0xfffull);

			for (; idx[2] < 512; idx[2] += 1, idx[3] = 0) {
				pages_consumed += check_needs_page(tables[2], idx[2], &free_pages);
				tables[3] = reinterpret_cast<page_table *>(
						tables[2]->entries[idx[2]] & ~0xfffull);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys_addr | 0xb;
					phys_addr += page_manager::page_size;
					if (--count == 0) return pages_consumed;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_in");
}

void
page_out(page_table *pml4, uint64_t virt_addr, uint64_t count)
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

