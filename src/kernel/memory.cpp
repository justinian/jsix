#include <stdint.h>

#include "kutil/enum_bitfields.h"
#include "assert.h"
#include "console.h"
#include "memory.h"

memory_manager *memory_manager::s_instance = nullptr;

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

	page_table * next(int i) const
	{
		return reinterpret_cast<page_table *>(entries[i] & 0xfffffffffffff000);
	}
};

struct page_block
{
	uint64_t physical_address;
	uint32_t count;
	uint32_t flags;
};

void
memory_manager::create(const void *memory_map, size_t map_length, size_t desc_length)
{
	// The bootloader reserved 4 pages for page tables, which we'll use to bootstrap.
	// The first one is the already-installed PML4, so grab it from CR3.
	page_table *tables = nullptr;
	__asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (tables) );

	// Now go through EFi's memory map and find a 4MiB region of free space to
	// use as a scratch space. We'll use the 2MiB that fits naturally aligned
	// into a single page table.
	efi_memory_descriptor const *free_mem =
		reinterpret_cast<efi_memory_descriptor const *>(memory_map);
	efi_memory_descriptor const *end = desc_incr(free_mem, map_length);

	while (free_mem < end) {
		if (free_mem->type == efi_memory_type::available && free_mem->pages >= 1024)
			break;

		free_mem = desc_incr(free_mem, desc_length);
		continue;
	}
	kassert(free_mem < end, "Couldn't find 4MiB of contiguous scratch space.");

	uint64_t start = (free_mem->physical_start & 0x1fffff) == 0 ?
		free_mem->physical_start :
		free_mem->physical_start + 0x1fffff & 0x1fffff;

	// Identity-map that region. We'll need to copy any existing tables (except
	// the PML4 which the bootloader gave us) into our 4 reserved pages so we
	// can edit them.
	uint64_t pml4_index = (start >> 39) & 0x1ff;
	uint64_t pdpt_index = (start >> 30) & 0x1ff;
	uint64_t pdt_index =  (start >> 21) & 0x1ff;

	if (tables[0].entries[pml4_index] & 0x1) {
		page_table *old_pdpt = tables[0].next(pml4_index);
		for (int i = 0; i < 512; ++i) tables[1].entries[i] = old_pdpt->entries[i];
	} else {
		for (int i = 0; i < 512; ++i) tables[1].entries[i] = 0;
	}
	tables[0].entries[pml4_index] = reinterpret_cast<uint64_t>(&tables[1]) | 0xb;

	if (tables[1].entries[pdpt_index] & 0x1) {
		page_table *old_pdt = tables[1].next(pdpt_index);
		for (int i = 0; i < 512; ++i) tables[2].entries[i] = old_pdt->entries[i];
	} else {
		for (int i = 0; i < 512; ++i) tables[2].entries[i] = 0;
	}
	tables[1].entries[pdpt_index] = reinterpret_cast<uint64_t>(&tables[2]) | 0xb;

	for (int i = 0; i < 512; ++i)
		tables[3].entries[i] = (start + 0x1000) | 0xb;
	tables[2].entries[pdt_index] = reinterpret_cast<uint64_t>(&tables[3]) | 0xb;

	// We now have 2MiB starting at "start" to bootstrap ourselves
	char const *hello = "Hello, beautiful memory! A little breathing space!";
	char *world = reinterpret_cast<char *>(start);
	while (*hello) *world++ = *hello++;
}

memory_manager::memory_manager(void *efi_runtime, void *memory_map, size_t map_length)
{
}
