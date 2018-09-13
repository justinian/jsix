#include <algorithm>

#include "kutil/assert.h"
#include "kutil/memory_manager.h"
#include "console.h"
#include "log.h"
#include "page_manager.h"

page_manager g_page_manager;
kutil::memory_manager g_kernel_memory_manager;


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


void mm_grow_callback(void *next, size_t length)
{
	kassert(length % page_manager::page_size == 0,
			"Heap manager requested a fractional page.");

	size_t pages = length / page_manager::page_size;
	log::info(logs::memory, "Heap manager growing heap by %d pages.", pages);
	g_page_manager.map_pages(reinterpret_cast<addr_t>(next), pages);
}


int
page_block::compare(const page_block *rhs) const
{
	if (virtual_address < rhs->virtual_address)
		return -1;
	else if (virtual_address > rhs->virtual_address)
		return 1;

	if (physical_address < rhs->physical_address)
		return -1;
	else if (physical_address > rhs->physical_address)
		return 1;

	return 0;
}

page_block_list
page_block::consolidate(page_block_list &list)
{
	page_block_list freed;

	for (auto *cur : list) {
		auto *next = cur->next();

		while (next &&
			cur->flags == next->flags &&
			cur->physical_end() == next->physical_address &&
			(!cur->has_flag(page_block_flags::mapped) ||
			cur->virtual_end() == next->virtual_address)) {

			cur->count += next->count;
			list.remove(next);
			freed.push_back(next);
		}
	}

	return freed;
}

void
page_block::dump(const page_block_list &list, const char *name, bool show_unmapped)
{
	log::info(logs::memory, "Block list %s:", name);

	int count = 0;
	for (auto *cur : list) {
		count += 1;
		if (!(show_unmapped || cur->has_flag(page_block_flags::mapped)))
			continue;

		if (cur->virtual_address) {
			page_table_indices start{cur->virtual_address};
			log::info(logs::memory, "  %016lx %08x [%6d] %016lx (%d,%d,%d,%d)",
					cur->physical_address,
					cur->flags,
					cur->count,
					cur->virtual_address,
					start[0], start[1], start[2], start[3]);
		} else {
			page_table_indices start{cur->virtual_address};
			log::info(logs::memory, "  %016lx %08x [%6d]",
					cur->physical_address,
					cur->flags,
					cur->count);
		}
	}

	log::info(logs::memory, "  Total: %d", count);
}

void
page_block::zero()
{
	physical_address = 0;
	virtual_address = 0;
	count = 0;
	flags = page_block_flags::free;
}

void
page_block::copy(page_block *other)
{
	physical_address = other->physical_address;
	virtual_address = other->virtual_address;
	count = other->count;
	flags = other->flags;
}


page_manager::page_manager() :
	m_block_slab(page_size),
	m_page_cache(nullptr)
{
	kassert(this == &g_page_manager, "Attempt to create another page_manager.");
}

void
page_manager::init(
	page_block_list free,
	page_block_list used,
	page_block_list cache)
{
	m_free.append(free);
	m_used.append(used);
	m_block_slab.append(cache);

	consolidate_blocks();

	// Initialize the kernel memory manager
	addr_t end = 0;
	for (auto *block : m_used) {
		if (block->virtual_address &&
			block->virtual_address < page_offset) {
			end = block->virtual_end();
		} else {
			break;
		}
	}

	new (&g_kernel_memory_manager) kutil::memory_manager(
			reinterpret_cast<void *>(end),
			mm_grow_callback);
	kutil::setup::set_heap(&g_kernel_memory_manager);

	m_kernel_pml4 = get_pml4();
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
	addr_t *p = reinterpret_cast<addr_t *>(pointer);
	addr_t v = *p + page_offset;
	addr_t c = ((length - 1) / page_size) + 1;

	// TODO: cleanly search/split this as a block out of used/free if possible
	auto *block = m_block_slab.pop();

	// TODO: page-align
	block->physical_address = *p;
	block->virtual_address = v;
	block->count = c;
	block->flags =
		page_block_flags::used |
		page_block_flags::mapped |
		page_block_flags::mmio;

	m_used.sorted_insert(block);

	page_table *pml4 = get_pml4();
	page_in(pml4, *p, v, c);
	*p = v;
}

void
page_manager::dump_blocks(bool used_only)
{
	page_block::dump(m_used, "used", true);
	if (!used_only)
		page_block::dump(m_free, "free", true);
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
		addr_t phys = 0;
		size_t n = pop_pages(32, &phys);
		addr_t virt = phys + page_offset;

		auto *block = m_block_slab.pop();

		block->physical_address = phys;
		block->virtual_address = virt;
		block->count = n;

		m_used.sorted_insert(block);

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

		log::debug(logs::memory, "Mappd %d new page table pages at %lx", n, phys);
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
	m_block_slab.append(page_block::consolidate(m_free));
	m_block_slab.append(page_block::consolidate(m_used));
}

void *
page_manager::map_pages(addr_t address, size_t count, bool user, page_table *pml4)
{
	void *ret = reinterpret_cast<void *>(address);
	if (!pml4) pml4 = get_pml4();

	while (count) {
		kassert(!m_free.empty(), "page_manager::map_pages ran out of free pages!");

		addr_t phys = 0;
		size_t n = pop_pages(count, &phys);

		auto *block = m_block_slab.pop();

		block->physical_address = phys;
		block->virtual_address = address;
		block->count = n;
		block->flags =
				page_block_flags::used |
				page_block_flags::mapped;

		m_used.sorted_insert(block);

		log::debug(logs::memory, "Paging in %d pages at p:%016lx to v:%016lx into %016lx table",
				n, phys, address, pml4);

		page_in(pml4, phys, address, n, user);

		address += n * page_size;
		count -= n;
	}

	return ret;
}

void *
page_manager::map_offset_pages(size_t count)
{
	page_table *pml4 = get_pml4();

	for (auto *free : m_free) {
		if (free->count < count) continue;

		auto *used = m_block_slab.pop();

		used->count = count;
		used->physical_address = free->physical_address;
		used->virtual_address = used->physical_address + page_offset;
		used->flags =
			page_block_flags::used |
			page_block_flags::mapped;

		m_used.sorted_insert(used);

		free->physical_address += count * page_size;
		free->count -= count;

		if (free->count == 0) {
			m_free.remove(free);
			free->zero();
			m_block_slab.push(free);
		}

		log::debug(logs::memory, "Got request for offset map %016lx [%d]", used->virtual_address, count);
		page_in(pml4, used->physical_address, used->virtual_address, count);
		return reinterpret_cast<void *>(used->virtual_address);
	}

	return nullptr;
}

void
page_manager::unmap_pages(void* address, size_t count)
{
	addr_t addr = reinterpret_cast<addr_t>(address);
	size_t block_count = 0;

	for (auto *block : m_used) {
		if (!block->contains(addr)) continue;

		size_t size = page_size * count;
		addr_t end = addr + size;

		size_t leading = addr - block->virtual_address;
		size_t trailing =
			end > block->virtual_end() ?
			0 : (block->virtual_end() - end);

		if (leading) {
			size_t pages = leading / page_size;

			auto *lead_block = m_block_slab.pop();

			lead_block->copy(block);
			lead_block->count = pages;

			block->count -= pages;
			block->physical_address += leading;
			block->virtual_address += leading;

			m_used.insert_before(block, lead_block);
		}

		if (trailing) {
			size_t pages = trailing / page_size;

			auto *trail_block = m_block_slab.pop();

			trail_block->copy(block);
			trail_block->count = pages;
			trail_block->physical_address += size;
			trail_block->virtual_address += size;

			block->count -= pages;

			m_used.insert_after(block, trail_block);
		}

		addr += block->count * page_size;

		block->virtual_address = 0;
		block->flags = block->flags &
			~(page_block_flags::used | page_block_flags::mapped);

		m_used.remove(block);
		m_free.sorted_insert(block);
		++block_count;
	}

	kassert(block_count, "Couldn't find existing mapped pages to unmap");
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
page_manager::page_in(page_table *pml4, addr_t phys_addr, addr_t virt_addr, size_t count, bool user)
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
	kassert(!m_free.empty(), "page_manager::pop_pages ran out of free pages!");

	auto *first = m_free.front();

	unsigned n = std::min(count, static_cast<size_t>(first->count));
	*address = first->physical_address;

	first->physical_address += n * page_size;
	first->count -= n;
	if (first->count == 0)
		m_block_slab.push(m_free.pop_front());

	return n;
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


