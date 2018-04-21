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

void
memory_manager::create(const void *memory_map, size_t map_length, size_t desc_length)
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

	uint64_t free_region = (desc->physical_start & 0x1fffff) == 0 ?
		desc->physical_start :
		desc->physical_start + 0x1fffff & ~0x1fffffull;

	// Offset-map this region into the higher half.
	uint64_t next_free = free_region + 0xffff800000000000;

	cons->puts("Found region: ");
	cons->put_hex(free_region);
	cons->puts("\n");

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

	for (int i = 0; i < 512; ++i)
		tables[3].entries[i] = (free_region + 0x1000 * i) | 0xb;
	tables[2].entries[fr_idx[2]] = reinterpret_cast<uint64_t>(&tables[3]) | 0xb;

	// We now have 2MiB starting at "free_region" to bootstrap ourselves. Start by
	// taking inventory of free pages.
	page_block *block_list = reinterpret_cast<page_block *>(next_free);

	int i = 0;
	page_block *free_head = nullptr, **free = &free_head;
	page_block *used_head = nullptr, **used = &used_head;

	desc = reinterpret_cast<efi_memory_descriptor const *>(memory_map);
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
			if (free_region >= block->physical_address && free_region < block->end()) {
				// This is the scratch memory block, split off what we're not using
				block->virtual_address = block->physical_address + 0xffff800000000000;

				block->flags = page_block_flags::used
					| page_block_flags::mapped
					| page_block_flags::pending_free;

				if (block->count > 1024) {
					page_block *rest = &block_list[i++];
					rest->physical_address = desc->physical_start + (1024*0x1000);
					rest->virtual_address = 0;
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

	// Update the pointer to the next free page
	next_free += i * sizeof(page_block);
	next_free = ((next_free - 1) & ~0xfffull) + 0x1000;

	// Now go back through these lists and consolidate
	free_head->list_consolidate();
	used_head->list_consolidate();

	// Ok, now build an acutal set of kernel page tables that just contains
	// what the kernel actually has mapped.
	unsigned table_page_count = count_table_pages_needed(used_head);

	cons->puts("To map currently-mapped pages, we need ");
	cons->put_dec(table_page_count);
	cons->puts(" pages of tables.\n");
}
