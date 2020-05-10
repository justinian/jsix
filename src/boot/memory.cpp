#include <stddef.h>
#include <uefi/types.h>
#include "kernel_args.h"

#include "console.h"
#include "error.h"
#include "memory.h"

namespace boot {
namespace memory {

size_t fixup_pointer_index = 0;
void **fixup_pointers[64];
uint64_t *new_pml4 = 0;

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

static const wchar_t *
memory_type_name(uefi::memory_type t)
{
	if (t < uefi::memory_type::max_memory_type) {
		return memory_type_names[static_cast<uint32_t>(t)];
	}

	switch(t) {
		case args_type:		return L"jsix kernel args";
		case module_type:	return L"jsix bootloader module";
		case kernel_type:	return L"jsix kernel code";
		case table_type:	return L"jsix page tables";
		default: return L"Bad Type Value";
	}
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

	try_or_raise(
		bs->create_event(
			uefi::evt::signal_virtual_address_change,
			uefi::tpl::callback,
			(uefi::event_notify)&update_marked_addresses,
			rs,
			&event),
		L"Error creating memory virtualization event");

	// Reserve a page for our replacement PML4, plus some pages for the kernel to use
	// as page tables while it gets started.
	void *addr = nullptr;
	try_or_raise(
		bs->allocate_pages(
			uefi::allocate_type::any_pages,
			table_type,
			64,
			&addr),
		L"Error allocating page table pages.");

	new_pml4 = reinterpret_cast<uint64_t*>(addr);
}

void
mark_pointer_fixup(void **p)
{
	if (fixup_pointer_index == 0) {
		const size_t count = sizeof(fixup_pointers) / sizeof(void*);
		for (size_t i = 0; i < count; ++i) fixup_pointers[i] = 0;
	}
	fixup_pointers[fixup_pointer_index++] = p;
}

/*
void
memory_virtualize(EFI_RUNTIME_SERVICES *runsvc, struct memory_map *map)
{
	memory_mark_pointer_fixup((void **)&runsvc);
	memory_mark_pointer_fixup((void **)&map);

	// Get the pointer to the start of PML4
	uint64_t* cr3 = 0;
	__asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (cr3) );

	// PML4 is indexed with bits 39:47 of the virtual address
	uint64_t offset = (KERNEL_VIRT_ADDRESS >> 39) & 0x1ff;

	// Double map the lower half pages that are present into the higher half
	for (unsigned i = 0; i < offset; ++i) {
		if (cr3[i] & 0x1)
			new_pml4[i] = new_pml4[offset+i] = cr3[i];
		else
			new_pml4[i] = new_pml4[offset+i] = 0;
	}

	// Write our new PML4 pointer back to CR3
	__asm__ __volatile__ ( "mov %0, %%cr3" :: "r" (new_pml4) );

	EFI_MEMORY_DESCRIPTOR *end = INCREMENT_DESC(map->entries, map->length);
	EFI_MEMORY_DESCRIPTOR *d = map->entries;
	while (d < end) {
		switch (d->Type) {
		case memtype_kernel:
		case memtype_data:
		case memtype_initrd:
		case memtype_scratch:
			d->Attribute |= EFI_MEMORY_RUNTIME;
			d->VirtualStart = d->PhysicalStart + KERNEL_VIRT_ADDRESS;

		default:
			if (d->Attribute & EFI_MEMORY_RUNTIME) {
				d->VirtualStart = d->PhysicalStart + KERNEL_VIRT_ADDRESS;
			}
		}
		d = INCREMENT_DESC(d, map->size);
	}

	runsvc->SetVirtualAddressMap(map->length, map->size, map->version, map->entries);
}
*/

efi_mem_map
get_mappings(uefi::boot_services *bs)
{
	size_t needs_size = 0;
	size_t map_key = 0;
	size_t desc_size = 0;
	uint32_t desc_version = 0;

	uefi::status status = bs->get_memory_map(
		&needs_size, nullptr, &map_key, &desc_size, &desc_version);

	if (status != uefi::status::buffer_too_small)
		error::raise(status, L"Error getting memory map size");

	console::print(L"memory map needs %lu bytes\r\n", needs_size);

	size_t buffer_size = needs_size + 10*desc_size;
	uefi::memory_descriptor *buffer = nullptr;
	try_or_raise(
		bs->allocate_pool(
			uefi::memory_type::loader_data, buffer_size,
			reinterpret_cast<void**>(&buffer)),
		L"Allocating space for memory map");

	try_or_raise(
		bs->get_memory_map(&buffer_size, buffer, &map_key, &desc_size, &desc_version),
		L"Getting UEFI memory map");

	efi_mem_map map;
	map.length = buffer_size;
	map.size = desc_size;
	map.key = map_key;
	map.version = desc_version;
	map.entries = buffer;

	for (auto desc : map) {
	//for(size_t i = 0; i < map.num_entries(); ++i) {
		//uefi::memory_descriptor *desc = map[i];
		console::print(L"   Range %lx (%lx) %x(%s) [%lu]\r\n",
			desc->physical_start, desc->attribute, desc->type, memory_type_name(desc->type), desc->number_of_pages);
	}

	return map;
}


} // namespace boot
} // namespace memory
