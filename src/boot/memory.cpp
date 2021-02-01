#include <stddef.h>
#include <uefi/types.h>

#include "kernel_memory.h"

#include "console.h"
#include "error.h"
#include "memory.h"
#include "paging.h"
#include "status.h"

namespace boot {
namespace memory {

using mem_entry = kernel::args::mem_entry;
using mem_type = kernel::args::mem_type;
using frame_block = kernel::args::frame_block;
using kernel::args::frames_per_block;

size_t fixup_pointer_index = 0;
void **fixup_pointers[64];

static const wchar_t *memory_type_names[] = {
	L"reserved memory type",
	L"loader code",
	L"loader data",
	L"boot services code",
	L"boot services data",
	L"runtime services code",
	L"runtime services data",
	L"conventional memory",
	L"unusable memory",
	L"acpi reclaim memory",
	L"acpi memory nvs",
	L"memory mapped io",
	L"memory mapped io port space",
	L"pal code",
	L"persistent memory"
};

static const wchar_t *kernel_memory_type_names[] = {
	L"free",
	L"pending",
	L"acpi",
	L"uefi_runtime",
	L"mmio",
	L"persistent"
};

static const wchar_t *
memory_type_name(uefi::memory_type t)
{
	if (t < uefi::memory_type::max_memory_type)
		return memory_type_names[static_cast<uint32_t>(t)];

	return L"Bad Type Value";
}

static const wchar_t *
kernel_memory_type_name(kernel::args::mem_type t)
{
	return kernel_memory_type_names[static_cast<uint32_t>(t)];
}

void
update_marked_addresses(uefi::event, void *context)
{
	uefi::runtime_services *rs =
		reinterpret_cast<uefi::runtime_services*>(context);

	for (size_t i = 0; i < fixup_pointer_index; ++i) {
		if (fixup_pointers[i])
			rs->convert_pointer(0, fixup_pointers[i]);
	}
}

void
init_pointer_fixup(uefi::boot_services *bs, uefi::runtime_services *rs)
{
	status_line status(L"Initializing pointer virtualization event");

	uefi::event event;
	bs->set_mem(&fixup_pointers, sizeof(fixup_pointers), 0);
	fixup_pointer_index = 0;

	try_or_raise(
		bs->create_event(
			uefi::evt::signal_virtual_address_change,
			uefi::tpl::callback,
			(uefi::event_notify)&update_marked_addresses,
			rs,
			&event),
		L"Error creating memory virtualization event");
}

void
mark_pointer_fixup(void **p)
{
	fixup_pointers[fixup_pointer_index++] = p;
}

bool
can_merge(mem_entry &prev, mem_type type, uefi::memory_descriptor *next)
{
	return
		prev.type == type &&
		prev.start + (page_size * prev.pages) == next->physical_start &&
		prev.attr == (next->attribute & 0xffffffff);
}

void
get_uefi_mappings(efi_mem_map &map, uefi::boot_services *bs)
{
	size_t length = map.total;
	uefi::status status = bs->get_memory_map(
		&length, map.entries, &map.key, &map.size, &map.version);
	map.length = length;

	if (status == uefi::status::success)
		return;

	if (status != uefi::status::buffer_too_small)
		error::raise(status, L"Error getting memory map size");

	if (map.entries) {
		try_or_raise(
			bs->free_pool(reinterpret_cast<void*>(map.entries)),
			L"Freeing previous memory map space");
	}

	map.total = length + 10*map.size;

	try_or_raise(
		bs->allocate_pool(
			uefi::memory_type::loader_data, map.total,
			reinterpret_cast<void**>(&map.entries)),
		L"Allocating space for memory map");

	map.length = map.total;
	try_or_raise(
		bs->get_memory_map(&map.length, map.entries, &map.key, &map.size, &map.version),
		L"Getting UEFI memory map");
}

inline size_t bitmap_size(size_t frames) { return (frames + 63) / 64; }
inline size_t num_blocks(size_t frames) { return (frames + (frames_per_block-1)) / frames_per_block; }

void
build_kernel_frame_blocks(const mem_entry *map, size_t nent, kernel::args::header *args, uefi::boot_services *bs)
{
	status_line status {L"Creating kernel frame accounting map"};

	size_t block_count = 0;
	size_t total_bitmap_size = 0;
	for (size_t i = 0; i < nent; ++i) {
		const mem_entry &ent = map[i];
		if (ent.type != mem_type::free)
			continue;

		block_count += num_blocks(ent.pages);
		total_bitmap_size += bitmap_size(ent.pages) * sizeof(uint64_t);
	}

	size_t total_size = block_count * sizeof(frame_block) + total_bitmap_size;

	frame_block *blocks = nullptr;
	try_or_raise(
		bs->allocate_pages(
			uefi::allocate_type::any_pages,
			uefi::memory_type::loader_data,
			bytes_to_pages(total_size),
			reinterpret_cast<void**>(&blocks)),
		L"Error allocating kernel frame block space");

	frame_block *next_block = blocks;
	for (size_t i = 0; i < nent; ++i) {
		const mem_entry &ent = map[i];
		if (ent.type != mem_type::free)
			continue;

		size_t page_count = ent.pages;
		uintptr_t base_addr = ent.start;
		while (page_count) {
			frame_block *blk = next_block++;
			bs->set_mem(blk, sizeof(frame_block), 0);

			blk->attrs = ent.attr;
			blk->base = base_addr;
			base_addr += frames_per_block * page_size;

			if (page_count >= frames_per_block) {
				page_count -= frames_per_block;
				blk->count = frames_per_block;
				blk->map1 = ~0ull;
				bs->set_mem(blk->map2, sizeof(blk->map2), 0xff);
			} else {
				blk->count = page_count;
				unsigned i = 0;

				uint64_t b1 = (page_count + 4095) / 4096;
				blk->map1 = (1 << b1) - 1;

				uint64_t b2 = (page_count + 63) / 64;
				uint64_t b2q = b2 / 64;
				uint64_t b2r = b2 % 64;
				bs->set_mem(blk->map2, b2q, 0xff);
				blk->map2[b2q] = (1 << b2r) - 1;
				break;
			}
		}
	}

	uint64_t *bitmap = reinterpret_cast<uint64_t*>(next_block);
	bs->set_mem(bitmap, total_bitmap_size, 0);
	for (unsigned i = 0; i < block_count; ++i) {
		frame_block &blk = blocks[i];
		blk.bitmap = bitmap;

		size_t b = blk.count / 64;
		size_t r = blk.count % 64;
		bs->set_mem(blk.bitmap, b*8, 0xff);
		blk.bitmap[b] = (1 << r) - 1;

		bitmap += bitmap_size(blk.count);
	}

	args->frame_block_count = block_count;
	args->frame_block_pages = bytes_to_pages(total_size);
	args->frame_blocks = blocks;
}

void
fix_frame_blocks(kernel::args::header *args)
{
	// Map the frame blocks to the appropriate address
	paging::map_pages(args,
		reinterpret_cast<uintptr_t>(args->frame_blocks),
		::memory::bitmap_start,
		args->frame_block_pages,
		true, false);

	uintptr_t offset = ::memory::bitmap_start -
		reinterpret_cast<uintptr_t>(args->frame_blocks);

	for (unsigned i = 0; i < args->frame_block_count; ++i) {
		frame_block &blk = args->frame_blocks[i];
		blk.bitmap = reinterpret_cast<uint64_t*>(
			reinterpret_cast<uintptr_t>(blk.bitmap) + offset);
	}
}

efi_mem_map
build_kernel_mem_map(kernel::args::header *args, uefi::boot_services *bs)
{
	status_line status {L"Creating kernel memory map"};

	efi_mem_map map;
	get_uefi_mappings(map, bs);

	size_t map_size = map.num_entries() * sizeof(mem_entry);

	kernel::args::mem_entry *kernel_map = nullptr;
	try_or_raise(
		bs->allocate_pages(
			uefi::allocate_type::any_pages,
			uefi::memory_type::loader_data,
			bytes_to_pages(map_size),
			reinterpret_cast<void**>(&kernel_map)),
		L"Error allocating kernel memory map module space");

	bs->set_mem(kernel_map, map_size, 0);
	get_uefi_mappings(map, bs);

	size_t nent = 0;
	bool first = true;
	for (auto desc : map) {
		/*
		// EFI map dump
		console::print(L"   eRange %lx (%lx) %x(%s) [%lu]\r\n",
			desc->physical_start, desc->attribute, desc->type, memory_type_name(desc->type), desc->number_of_pages);
		*/

		mem_type type;
		switch (desc->type) {
			case uefi::memory_type::reserved:
			case uefi::memory_type::unusable_memory:
			case uefi::memory_type::acpi_memory_nvs:
			case uefi::memory_type::pal_code:
				continue;

			case uefi::memory_type::loader_code:
			case uefi::memory_type::boot_services_code:
			case uefi::memory_type::boot_services_data:
			case uefi::memory_type::conventional_memory:
			case uefi::memory_type::loader_data:
				type = mem_type::free;
				break;

			case uefi::memory_type::runtime_services_code:
			case uefi::memory_type::runtime_services_data:
				type = mem_type::uefi_runtime;
				break;

			case uefi::memory_type::acpi_reclaim_memory:
				type = mem_type::acpi;
				break;

			case uefi::memory_type::memory_mapped_io:
			case uefi::memory_type::memory_mapped_io_port_space:
				type = mem_type::mmio;
				break;

			case uefi::memory_type::persistent_memory:
				type = mem_type::persistent;
				break;

			default:
				error::raise(
					uefi::status::invalid_parameter,
					L"Got an unexpected memory type from UEFI memory map");
		}

		// TODO: validate uefi's map is sorted
		if (first) {
			first = false;
			mem_entry &ent = kernel_map[nent++];
			ent.start = desc->physical_start;
			ent.pages = desc->number_of_pages;
			ent.type = type;
			ent.attr = (desc->attribute & 0xffffffff);
			continue;
		}

		mem_entry &prev = kernel_map[nent - 1];
		if (can_merge(prev, type, desc)) {
			prev.pages += desc->number_of_pages;
		} else {
			mem_entry &next = kernel_map[nent++];
			next.start = desc->physical_start;
			next.pages = desc->number_of_pages;
			next.type = type;
			next.attr = (desc->attribute & 0xffffffff);
		}
	}

	// Give just the actually-set entries in the header
	args->mem_map = kernel_map;
	args->map_count = nent;

	/*
	// kernel map dump
	for (unsigned i = 0; i < nent; ++i) {
		const kernel::args::mem_entry &e = kernel_map[i];
		console::print(L"   kRange %lx (%lx) %x(%s) [%lu]\r\n",
			e.start, e.attr, e.type, kernel_memory_type_name(e.type), e.pages);
	}
	*/

	build_kernel_frame_blocks(kernel_map, nent, args, bs);
	get_uefi_mappings(map, bs);
	return map;
}

void
virtualize(void *pml4, efi_mem_map &map, uefi::runtime_services *rs)
{
	paging::add_current_mappings(reinterpret_cast<paging::page_table*>(pml4));

	for (auto desc : map)
		desc->virtual_start = desc->physical_start + ::memory::page_offset;

	// Write our new PML4 pointer to CR3
	asm volatile ( "mov %0, %%cr3" :: "r" (pml4) );
	__sync_synchronize();

	try_or_raise(
		rs->set_virtual_address_map(
			map.length, map.size, map.version, map.entries),
		L"Error setting virtual address map");
}


} // namespace boot
} // namespace memory
