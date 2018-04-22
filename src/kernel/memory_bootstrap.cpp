#include "kutil/memory.h"
#include "assert.h"
#include "console.h"
#include "memory.h"
#include "memory_pages.h"

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

struct page_table
{
	uint64_t entries[512];
	page_table * next(int i) const { return reinterpret_cast<page_table *>(entries[i] & ~0xfffull); }
};

static unsigned
count_table_pages_needed(page_block *used)
{
	page_table_indices last_idx{~0ull};
	unsigned counts[] = {1, 0, 0, 0};

	for (page_block *cur = used; cur; cur = cur->next) {
		if (!cur->has_flag(page_block_flags::mapped))
			continue;

		page_table_indices start{cur->virtual_address};
		page_table_indices end{cur->virtual_address + (cur->count * 0x1000)};

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

uint64_t
gather_block_lists(
		uint64_t scratch,
		const void *memory_map,
		size_t map_length,
		size_t desc_length,
		page_block **free_head,
		page_block **used_head)
{
	int i = 0;
	page_block **free = free_head;
	page_block **used = used_head;

	page_block *block_list = reinterpret_cast<page_block *>(scratch);
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
			if (scratch >= block->physical_address && scratch < block->physical_end()) {
				// This is the scratch memory block, split off what we're not using
				block->virtual_address = block->physical_address + 0xffff800000000000;
				block->flags = page_block_flags::used | page_block_flags::mapped;

				if (block->count > 1024) {
					page_block *rest = &block_list[i++];
					rest->physical_address = desc->physical_start + (1024*0x1000);
					rest->virtual_address = 0;
					rest->flags = page_block_flags::free;
					rest->count = desc->pages - 1024;
					rest->next = nullptr;
					*free = rest;
					free = &rest->next;

					block->count = 1024;
				}
			} else {
				block->flags = page_block_flags::free;
			}
			break;

		case efi_memory_type::acpi_reclaim:
			block->flags = page_block_flags::used | page_block_flags::acpi_wait;
			break;

		case efi_memory_type::persistent:
			block->flags = page_block_flags::nonvolatile;
			break;

		default:
			block->flags = page_block_flags::used | page_block_flags::permanent;
			break;
		}

		if (block->has_flag(page_block_flags::used)) {
			if (block->virtual_address != 0)
				block->flags |= page_block_flags::mapped;
			*used = block;
			used = &block->next;
		} else {
			*free = block;
			free = &block->next;
		}

		desc = desc_incr(desc, desc_length);
	}

	return reinterpret_cast<uint64_t>(&block_list[i]);
}

unsigned check_needs_page(page_table *table, unsigned index, page_table **free_pages)
{
	if (table->entries[index] & 0x1 == 1) return 0;

	kassert(*free_pages, "check_needs_page needed to allocate but had no free pages");

	page_table *new_table = (*free_pages)++;
	for (int i=0; i<512; ++i) new_table->entries[i] = 0;
	table->entries[index] = reinterpret_cast<uint64_t>(new_table) | 0xb;
	return 1;
}

unsigned page_in(page_table *pml4, uint64_t phys_addr, uint64_t virt_addr, uint64_t count, page_table *free_pages)
{
	page_table_indices idx{virt_addr};
	page_table *tables[4] = {pml4, nullptr, nullptr, nullptr};

	unsigned pages_consumed = 0;
	for (; idx[0] < 512; idx[0] += 1) {
		pages_consumed += check_needs_page(tables[0], idx[0], &free_pages);
		tables[1] = reinterpret_cast<page_table *>(
				tables[0]->entries[idx[0]] & ~0xfffull);

		for (; idx[1] < 512; idx[1] += 1) {
			pages_consumed += check_needs_page(tables[1], idx[1], &free_pages);
			tables[2] = reinterpret_cast<page_table *>(
					tables[1]->entries[idx[1]] & ~0xfffull);

			for (; idx[2] < 512; idx[2] += 1) {
				pages_consumed += check_needs_page(tables[2], idx[2], &free_pages);
				tables[3] = reinterpret_cast<page_table *>(
						tables[2]->entries[idx[2]] & ~0xfffull);

				for (; idx[3] < 512; idx[3] += 1) {
					tables[3]->entries[idx[3]] = phys_addr | 0xb;
					phys_addr += 0x1000;
					if (--count == 0) return pages_consumed;
				}
			}
		}
	}

	kassert(0, "Ran to end of page_in");
}

page_block *
fill_page_with_blocks(uint64_t start) {
	uint64_t space = page_align(start) - start;
	uint64_t count = space / sizeof(page_block);
	page_block *blocks = reinterpret_cast<page_block *>(start);
	kutil::memset(blocks, 0, sizeof(page_block)*count);

	page_block *head = nullptr, **insert = &head;
	for (unsigned i = 0; i < count; ++i) {
		*insert = &blocks[i];
		insert = &blocks[i].next;
	}

	return head;
}

void
memory_initialize_managers(const void *memory_map, size_t map_length, size_t desc_length)
{
	console *cons = console::get();

	// The bootloader reserved 4 pages for page tables, which we'll use to bootstrap.
	// The first one is the already-installed PML4, so grab it from CR3.
	page_table *tables = nullptr;
	__asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (tables) );

	// Now go through EFi's memory map and find a 4MiB region of free space to
	// use as a scratch space. We'll use the 2MiB that fits naturally aligned
	// into a single page table.
	efi_memory_descriptor const *desc =
		reinterpret_cast<efi_memory_descriptor const *>(memory_map);
	efi_memory_descriptor const *end = desc_incr(desc, map_length);

	while (desc < end) {
		if (desc->type == efi_memory_type::available && desc->pages >= 1024)
			break;

		desc = desc_incr(desc, desc_length);
	}
	kassert(desc < end, "Couldn't find 4MiB of contiguous scratch space.");

	// Offset-map this region into the higher half.
	uint64_t free_region_start = desc->physical_start;
	uint64_t free_region = page_table_align(free_region_start);
	uint64_t next_free = free_region + 0xffff800000000000;
	cons->puts("Skipping ");
	cons->put_dec(free_region - free_region_start);
	cons->puts(" bytes to get page-table-aligned.\n");

	// We'll need to copy any existing tables (except the PML4 which the
	// bootloader gave us) into our 4 reserved pages so we can edit them.
	page_table_indices fr_idx{free_region};
	fr_idx[0] += 256; // Flip the highest bit of the address

	if (tables[0].entries[fr_idx[0]] & 0x1) {
		page_table *old_pdpt = tables[0].next(fr_idx[0]);
		for (int i = 0; i < 512; ++i) tables[1].entries[i] = old_pdpt->entries[i];
	} else {
		for (int i = 0; i < 512; ++i) tables[1].entries[i] = 0;
	}
	tables[0].entries[fr_idx[0]] = reinterpret_cast<uint64_t>(&tables[1]) | 0xb;

	if (tables[1].entries[fr_idx[1]] & 0x1) {
		page_table *old_pdt = tables[1].next(fr_idx[1]);
		for (int i = 0; i < 512; ++i) tables[2].entries[i] = old_pdt->entries[i];
	} else {
		for (int i = 0; i < 512; ++i) tables[2].entries[i] = 0;
	}
	tables[1].entries[fr_idx[1]] = reinterpret_cast<uint64_t>(&tables[2]) | 0xb;

	// No need to copy the last-level page table, we're overwriting the whole thing
	tables[2].entries[fr_idx[2]] = reinterpret_cast<uint64_t>(&tables[3]) | 0xb;
	page_in(&tables[0], free_region, next_free, 512, nullptr);

	// We now have 2MiB starting at "free_region" to bootstrap ourselves. Start by
	// taking inventory of free pages.
	page_block *free_head = nullptr;
	page_block *used_head = nullptr;
	next_free = gather_block_lists(next_free, memory_map, map_length, desc_length,
			&free_head, &used_head);

	// Unused page_block structs go here - finish out the current page with them
	page_block *cache_head = fill_page_with_blocks(next_free);
	next_free = page_align(next_free);

	// Now go back through these lists and consolidate
	page_block **cache = &cache_head;
	*cache = free_head->list_consolidate();
	while (*cache) cache = &(*cache)->next;
	*cache = used_head->list_consolidate();

	// Ok, now build an acutal set of kernel page tables that just contains
	// what the kernel actually has mapped.
	page_table *pages = reinterpret_cast<page_table *>(next_free);
	unsigned consumed_pages = 1; // We're about to make a PML4, start with 1:w

	// Finally, remap the existing mappings, but making everything writable
	// (especially the page tables themselves)
	page_table *pml4 = pages++;
	for (int i=0; i<512; ++i) pml4->entries[i] = 0;

	for (page_block *cur = used_head; cur; cur = cur->next) {
		if (!cur->has_flag(page_block_flags::mapped)) continue;
		consumed_pages += page_in(pml4, cur->physical_address, cur->virtual_address,
				cur->count, pages + consumed_pages);
	}
	next_free += (consumed_pages * 0x1000);

	// We now have all used memory mapped ourselves. Let the page_manager take
	// over from here.
	page_manager::init(
			free_head, used_head, cache_head,
			free_region_start, 1024 * 0x1000, next_free);
}
