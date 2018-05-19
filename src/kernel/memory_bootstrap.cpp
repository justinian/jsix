#include "kutil/assert.h"
#include "kutil/memory.h"
#include "memory.h"
#include "page_manager.h"

const unsigned efi_page_size = 0x1000;
const unsigned ident_page_flags = 0xf; // TODO: set to 0xb when user/kernel page tables are better sorted

enum class efi_memory_type : uint32_t
{
	reserved,
	loader_code,
	loader_data,
	boot_services_code,
	boot_services_data,
	runtime_services_code,
	runtime_services_data,
	available,
	unusable,
	acpi_reclaim,
	acpi_nvs,
	mmio,
	mmio_port,
	pal_code,
	persistent,

	efi_max,

	popcorn_kernel = 0x80000000,
	popcorn_font,
	popcorn_data,
	popcorn_log,
	popcorn_pml4,

	popcorn_max
};

const char *efi_memory_type_names[] = {
	"             reserved",
	"          loader_code",
	"          loader_data",
	"   boot_services_code",
	"   boot_services_data",
	"runtime_services_code",
	"runtime_services_data",
	"            available",
	"             unusable",
	"         acpi_reclaim",
	"             acpi_nvs",
	"                 mmio",
	"            mmio_port",
	"             pal_code",

	"       popcorn_kernel",
	"         popcorn_font",
	"         popcorn_data",
	"          popcorn_log",
	"         popcorn_pml4",
};

static const char *
get_efi_name(efi_memory_type t)
{
	static const unsigned offset =
		(unsigned)efi_memory_type::popcorn_kernel - (unsigned)efi_memory_type::efi_max;

	return t >= efi_memory_type::popcorn_kernel ?
		efi_memory_type_names[(unsigned)t - offset] :
		efi_memory_type_names[(unsigned)t];
}

enum class efi_memory_flag : uint64_t
{
	can_mark_uc  = 0x0000000000000001, // uc = un-cacheable
	can_mark_wc  = 0x0000000000000002, // wc = write-combining
	can_mark_wt  = 0x0000000000000004, // wt = write through
	can_mark_wb  = 0x0000000000000008, // wb = write back
	can_mark_uce = 0x0000000000000010, // uce = un-cacheable exported
	can_mark_wp  = 0x0000000000001000, // wp = write protected
	can_mark_rp  = 0x0000000000002000, // rp = read protected
	can_mark_xp  = 0x0000000000004000, // xp = exceute protected
	can_mark_ro  = 0x0000000000020000, // ro = read only

	non_volatile  = 0x0000000000008000,
	more_reliable = 0x0000000000010000,
	runtime       = 0x8000000000000000
};
IS_BITFIELD(efi_memory_flag);

struct efi_memory_descriptor
{
	efi_memory_type type;
	uint32_t pad;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t pages;
	efi_memory_flag flags;
};

static const efi_memory_descriptor *
desc_incr(const efi_memory_descriptor *d, size_t desc_length)
{
	return reinterpret_cast<const efi_memory_descriptor *>(
			reinterpret_cast<const uint8_t *>(d) + desc_length);
}

static unsigned
count_table_pages_needed(page_block *used)
{
	page_table_indices last_idx{~0ull};
	unsigned counts[] = {1, 0, 0, 0};

	for (page_block *cur = used; cur; cur = cur->next) {
		if (!cur->has_flag(page_block_flags::mapped))
			continue;

		page_table_indices start{cur->virtual_address};
		page_table_indices end{cur->virtual_address + (cur->count * page_manager::page_size)};

		counts[1] +=
			((start[0] == last_idx[0]) ? 0 : 1) +
			(end[0] - start[0]);

		counts[2] +=
			((start[0] == last_idx[0] &&
			 start[1] == last_idx[1]) ? 0 : 1) +
			(end[1] - start[1]);

		counts[3] +=
			((start[0] == last_idx[0] &&
			 start[1] == last_idx[1] &&
			 start[2] == last_idx[2]) ? 0 : 1) +
			(end[2] - start[2]);

		last_idx = end;
	}

	return counts[0] + counts[1] + counts[2] + counts[3];

}

page_block *
remove_block_for(page_block **list, uint64_t phys_start, uint64_t pages, page_block **cache)
{
	// This is basically just the removal portion of page_manager::unmap_pages,
	// but with physical addresses, and only ever removing a single block.

	page_block *prev = nullptr;
	page_block *cur = *list;
	while (cur && !cur->contains_physical(phys_start)) {
		prev = cur;
		cur = cur->next;
	}

	kassert(cur, "Couldn't find block to remove");

	uint64_t size = page_manager::page_size * pages;
	uint64_t end = phys_start + size;
	uint64_t leading = phys_start - cur->physical_address;
	uint64_t trailing = cur->physical_end() - end;

	if (leading) {
		uint64_t pages = leading / page_manager::page_size;

		page_block *lead_block = *cache;
		*cache = (*cache)->next;

		lead_block->copy(cur);
		lead_block->next = cur;
		lead_block->count = pages;

		cur->count -= pages;
		cur->physical_address += leading;

		if (cur->virtual_address)
			cur->virtual_address += leading;

		if (prev) {
			prev->next = lead_block;
		} else {
			prev = lead_block;
			*list = prev;
		}
	}

	if (trailing) {
		uint64_t pages = trailing / page_manager::page_size;

		page_block *trail_block = *cache;
		*cache = (*cache)->next;

		trail_block->copy(cur);
		trail_block->next = cur->next;
		trail_block->count = pages;
		trail_block->physical_address += size;

		if (cur->virtual_address)
			trail_block->virtual_address += size;

		cur->count -= pages;
		cur->next = trail_block;
	}

	prev->next = cur->next;
	cur->next = nullptr;
	return cur;
}

uint64_t
gather_block_lists(
		uint64_t scratch_virt,
		const void *memory_map,
		size_t map_length,
		size_t desc_length,
		page_block **free_head,
		page_block **used_head)
{
	int i = 0;
	page_block *free = nullptr;
	page_block *used = nullptr;

	page_block *block_list = reinterpret_cast<page_block *>(scratch_virt);
	efi_memory_descriptor const *desc = reinterpret_cast<efi_memory_descriptor const *>(memory_map);
	efi_memory_descriptor const *end = desc_incr(desc, map_length);

	while (desc < end) {
		page_block *block = &block_list[i++];
		block->physical_address = desc->physical_start;
		block->virtual_address = desc->virtual_start;
		block->count = desc->pages;
		block->next = nullptr;

		switch (desc->type) {
		case efi_memory_type::loader_code:
		case efi_memory_type::loader_data:
			block->flags = page_block_flags::used | page_block_flags::pending_free;
			break;

		case efi_memory_type::boot_services_code:
		case efi_memory_type::boot_services_data:
		case efi_memory_type::available:
			block->flags = page_block_flags::free;
			break;

		case efi_memory_type::acpi_reclaim:
			block->flags =
				page_block_flags::used |
				page_block_flags::mapped |
				page_block_flags::acpi_wait;

			block->virtual_address = block->physical_address;
			break;

		case efi_memory_type::persistent:
			block->flags = page_block_flags::nonvolatile;
			break;

		default:
			block->flags = page_block_flags::used | page_block_flags::permanent;
			break;
		}

		if (block->has_flag(page_block_flags::used)) {
			if (block->virtual_address || !block->physical_address)
				block->flags |= page_block_flags::mapped;

			used = page_block::insert(used, block);
		} else {
			free = page_block::insert(free, block);
		}

		desc = desc_incr(desc, desc_length);
	}

	*free_head = free;
	*used_head = used;
	return reinterpret_cast<uint64_t>(&block_list[i]);
}

page_block *
fill_page_with_blocks(uint64_t start)
{
	uint64_t end = page_align(start);
	uint64_t count = (end - start) / sizeof(page_block);
	if (count == 0) return nullptr;

	page_block *blocks = reinterpret_cast<page_block *>(start);
	for (unsigned i = 0; i < count; ++i)
		blocks[i].zero(&blocks[i+1]);
	blocks[count - 1].next = nullptr;
	return blocks;
}

void
copy_new_table(page_table *base, unsigned index, page_table *new_table)
{
	uint64_t entry = base->entries[index];

	// If this is a large page and not a a table, bail out.
	if(entry & 0x80) return;

	if (entry & 0x1) {
		page_table *old_next = reinterpret_cast<page_table *>(
				base->entries[index] & ~0xffful);
		for (int i = 0; i < 512; ++i) new_table->entries[i] = old_next->entries[i];
	} else {
		for (int i = 0; i < 512; ++i) new_table->entries[i] = 0;
	}

	base->entries[index] = reinterpret_cast<uint64_t>(new_table) | ident_page_flags;
}

static uint64_t
find_efi_free_aligned_pages(const void *memory_map, size_t map_length, size_t desc_length, unsigned pages)
{
	efi_memory_descriptor const *desc =
		reinterpret_cast<efi_memory_descriptor const *>(memory_map);
	efi_memory_descriptor const *end = desc_incr(desc, map_length);

	const unsigned want_space = pages * page_manager::page_size;
	uint64_t start_phys = 0;

	for (; desc < end; desc = desc_incr(desc, desc_length)) {
		if (desc->type != efi_memory_type::available)
			continue;

		// See if the first wanted pages fit in one page table. If we
		// find free memory at zero, skip ahead because we're not ready
		// to deal with 0 being a valid pointer yet.
		start_phys = desc->physical_start;
		if (start_phys == 0)
			start_phys += efi_page_size;

		const uint64_t desc_end =
			desc->physical_start + desc->pages * efi_page_size;

		uint64_t end = start_phys + want_space;
		if (end < desc_end) {
			page_table_indices start_idx{start_phys};
			page_table_indices end_idx{end};
			if (start_idx[0] == end_idx[0] &&
				start_idx[1] == end_idx[1] &&
				start_idx[2] == end_idx[2])
				break;

			// Try seeing if the page-table-aligned version fits
			start_phys = page_table_align(start_phys);
			end = start_phys + want_space;
			if (end < desc_end)
				break;
		}
	}

	kassert(desc < end, "Couldn't find wanted pages of aligned scratch space.");
	return start_phys;
}

static unsigned
check_needs_page_ident(page_table *table, unsigned index, page_table **free_pages)
{
	if ((table->entries[index] & 0x1) == 1) return 0;

	kassert(*free_pages, "check_needs_page_ident needed to allocate but had no free pages");

	page_table *new_table = (*free_pages)++;
	for (int i=0; i<512; ++i) new_table->entries[i] = 0;
	table->entries[index] = reinterpret_cast<uint64_t>(new_table) | ident_page_flags;
	return 1;
}

static unsigned
page_in_ident(
		page_table *pml4,
		uint64_t phys_addr,
		uint64_t virt_addr,
		uint64_t count,
		page_table *free_pages)
{
	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	unsigned pages_consumed = 0;
	for (; idx[0] < 512; idx[0] += 1) {
		pages_consumed += check_needs_page_ident(tables[0], idx[0], &free_pages);
		tables[1] = reinterpret_cast<page_table *>(
				tables[0]->entries[idx[0]] & ~0xfffull);

		for (; idx[1] < 512; idx[1] += 1, idx[2] = 0, idx[3] = 0) {
			pages_consumed += check_needs_page_ident(tables[1], idx[1], &free_pages);
			tables[2] = reinterpret_cast<page_table *>(
					tables[1]->entries[idx[1]] & ~0xfffull);

			for (; idx[2] < 512; idx[2] += 1, idx[3] = 0) {
				if (idx[3] == 0 &&
					count >= 512 &&
					tables[2]->get(idx[2]) == nullptr) {
					// Do a 2MiB page instead
					tables[2]->entries[idx[2]] = phys_addr | 0x80 | ident_page_flags;

					phys_addr += page_manager::page_size * 512;
					count -= 512;
					if (count == 0) return pages_consumed;
					continue;
				}

				pages_consumed += check_needs_page_ident(tables[2], idx[2], &free_pages);
				tables[3] = reinterpret_cast<page_table *>(
						tables[2]->entries[idx[2]] & ~0xfffull);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys_addr | ident_page_flags;
					phys_addr += page_manager::page_size;
					if (--count == 0) return pages_consumed;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_in_ident");
	return 0; // Cannot reach
}

void
memory_initialize(const void *memory_map, size_t map_length, size_t desc_length)
{
	// The bootloader reserved 16 pages for page tables, which we'll use to bootstrap.
	// The first one is the already-installed PML4, so grab it from CR3.
	uint64_t cr3;
	__asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (cr3) );
	page_table *tables = reinterpret_cast<page_table *>(cr3 & ~0xfffull);

	// Now go through EFi's memory map and find a region of scratch space.
	const unsigned want_pages = 32;
	uint64_t free_region_start_phys =
		find_efi_free_aligned_pages(memory_map, map_length, desc_length, want_pages);

	// Offset-map this region into the higher half.
	uint64_t free_region_start_virt =
		free_region_start_phys + page_manager::high_offset;

	uint64_t free_next = free_region_start_virt;

	// We'll need to copy any existing tables (except the PML4 which the
	// bootloader gave us) into our 4 reserved pages so we can edit them.
	page_table_indices fr_idx{free_region_start_virt};

	copy_new_table(&tables[0], fr_idx[0], &tables[1]);
	copy_new_table(&tables[1], fr_idx[1], &tables[2]);
	copy_new_table(&tables[2], fr_idx[2], &tables[3]);
	page_in_ident(&tables[0], free_region_start_phys, free_region_start_virt, want_pages, nullptr);

	// We now have pages starting at "free_next" to bootstrap ourselves. Start by
	// taking inventory of free pages.
	page_block *free_head = nullptr;
	page_block *used_head = nullptr;
	free_next = gather_block_lists(
			free_next, memory_map, map_length, desc_length,
			&free_head, &used_head);

	// Unused page_block structs go here - finish out the current page with them
	page_block *cache_head = fill_page_with_blocks(free_next);
	free_next = page_align(free_next);

	// Now go back through these lists and consolidate
	page_block *freed = page_block::consolidate(free_head);
	cache_head = page_block::append(cache_head, freed);

	freed = page_block::consolidate(used_head);
	cache_head = page_block::append(cache_head, freed);


	// Pull out the block that represents the bootstrap pages we've used
	uint64_t used = free_next - free_region_start_virt;
	uint64_t used_pages = used / page_manager::page_size;
	uint64_t remaining_pages = want_pages - used_pages;

	page_block *removed = remove_block_for(
			&free_head,
			free_region_start_phys,
			used_pages,
			&cache_head);

	kassert(removed, "remove_block_for didn't find the bootstrap region.");
	kassert(removed->physical_address == free_region_start_phys,
			"remove_block_for found the wrong region.");

	// Add it to the used list
	removed->virtual_address = free_region_start_virt;
	removed->flags = page_block_flags::used | page_block_flags::mapped;
	used_head = page_block::insert(used_head, removed);

	// Pull out the block that represents the rest
	uint64_t free_next_phys = free_region_start_phys + used;

	removed = remove_block_for(
			&free_head,
			free_next_phys,
			remaining_pages,
			&cache_head);

	kassert(removed, "remove_block_for didn't find the page table region.");
	kassert(removed->physical_address == free_next_phys,
			"remove_block_for found the wrong region.");

	uint64_t pt_start_phys = removed->physical_address;
	uint64_t pt_start_virt = removed->physical_address + page_manager::page_offset;

	// Record that we're about to remap it into the page table address space
	removed->virtual_address = pt_start_virt;
	removed->flags = page_block_flags::used | page_block_flags::mapped;
	used_head = page_block::insert(used_head, removed);

	page_manager *pm = &g_page_manager;

	// Actually remap them into page table space
	pm->page_out(&tables[0], free_next, remaining_pages);

	page_table_indices pg_idx{pt_start_virt};
	copy_new_table(&tables[0], pg_idx[0], &tables[4]);
	copy_new_table(&tables[4], pg_idx[1], &tables[5]);
	copy_new_table(&tables[5], pg_idx[2], &tables[6]);

	page_in_ident(&tables[0], pt_start_phys, pt_start_virt, remaining_pages, tables + 4);

	// Finally, build an acutal set of kernel page tables that just contains
	// what the kernel actually has mapped, but making everything writable
	// (especially the page tables themselves)
	page_table *pml4 = reinterpret_cast<page_table *>(pt_start_virt);
	for (int i=0; i<512; ++i) pml4->entries[i] = 0;

	// Give the rest to the page_manager's cache for use in page_in
	pm->free_table_pages(pml4 + 1, remaining_pages - 1);

	for (page_block *cur = used_head; cur; cur = cur->next) {
		if (!cur->has_flag(page_block_flags::mapped)) continue;
		pm->page_in(pml4, cur->physical_address, cur->virtual_address, cur->count);
	}

	// Put our new PML4 into CR3 to start using it
	page_manager::set_pml4(pml4);

	// We now have all used memory mapped ourselves. Let the page_manager take
	// over from here.
	g_page_manager.init(free_head, used_head, cache_head);
}
