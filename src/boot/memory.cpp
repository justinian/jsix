#include <stddef.h>
#include <uefi/types.h>

#include "error.h"
#include "console.h"
#include "memory.h"
#include "utility.h"

namespace boot {
namespace memory {

/*
const EFI_MEMORY_TYPE memtype_kernel  = static_cast<EFI_MEMORY_TYPE>(0x80000000ul);
const EFI_MEMORY_TYPE memtype_data    = static_cast<EFI_MEMORY_TYPE>(0x80000001ul);
const EFI_MEMORY_TYPE memtype_initrd  = static_cast<EFI_MEMORY_TYPE>(0x80000002ul);
const EFI_MEMORY_TYPE memtype_scratch = static_cast<EFI_MEMORY_TYPE>(0x80000003ul);

#define INCREMENT_DESC(p, b)  (EFI_MEMORY_DESCRIPTOR*)(((uint8_t*)(p))+(b))
*/

size_t fixup_pointer_index = 0;
void **fixup_pointers[64];
uint64_t *new_pml4 = 0;

/*
const wchar_t *memory_type_names[] = {
	L"EfiReservedMemoryType",
	L"EfiLoaderCode",
	L"EfiLoaderData",
	L"EfiBootServicesCode",
	L"EfiBootServicesData",
	L"EfiRuntimeServicesCode",
	L"EfiRuntimeServicesData",
	L"EfiConventionalMemory",
	L"EfiUnusableMemory",
	L"EfiACPIReclaimMemory",
	L"EfiACPIMemoryNVS",
	L"EfiMemoryMappedIO",
	L"EfiMemoryMappedIOPortSpace",
	L"EfiPalCode",
	L"EfiPersistentMemory",
};

static const wchar_t *
memory_type_name(UINT32 value)
{
	if (value >= (sizeof(memory_type_names) / sizeof(wchar_t *))) {
		switch (value) {
			case memtype_kernel:  return L"Kernel Data";
			case memtype_data:    return L"Kernel Data";
			case memtype_initrd:  return L"Initial Ramdisk";
			case memtype_scratch: return L"Kernel Scratch Space";
			default: return L"Bad Type Value";
		}
	}
	return memory_type_names[value];
}
*/

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

	uefi::memory_type memtype = static_cast<uefi::memory_type>(0x80000003ul);

	// Reserve a page for our replacement PML4, plus some pages for the kernel to use
	// as page tables while it gets started.
	uintptr_t addr = 0;
	try_or_raise(
		bs->allocate_pages(
			uefi::allocate_type::any_pages,
			memtype,
			64,
			&addr),
		L"Error allocating page table pages.");

	new_pml4 = (uint64_t *)addr;
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
copy_desc(EFI_MEMORY_DESCRIPTOR *src, EFI_MEMORY_DESCRIPTOR *dst, size_t len)
{
	uint8_t *srcb = (uint8_t *)src;
	uint8_t *dstb = (uint8_t *)dst;
	uint8_t *endb = srcb + len;
	while (srcb < endb)
		*dstb++ = *srcb++;
}

EFI_STATUS
memory_get_map_length(EFI_BOOT_SERVICES *bootsvc, size_t *size)
{
	if (size == NULL)
		return EFI_INVALID_PARAMETER;

	EFI_STATUS status;
	size_t key, desc_size;
	uint32_t desc_version;
	*size = 0;
	status = bootsvc->GetMemoryMap(size, 0, &key, &desc_size, &desc_version);
	if (status != EFI_BUFFER_TOO_SMALL) {
		CHECK_EFI_STATUS_OR_RETURN(status, "Failed to get memory map size");
	}
	return EFI_SUCCESS;
}

EFI_STATUS
memory_get_map(EFI_BOOT_SERVICES *bootsvc, struct memory_map *map)
{
	EFI_STATUS status;

	if (map == NULL)
		return EFI_INVALID_PARAMETER;

	size_t needs_size = 0;
	status = memory_get_map_length(bootsvc, &needs_size);
	if (EFI_ERROR(status)) return status;

	if (map->length < needs_size)
		return EFI_BUFFER_TOO_SMALL;

	status = bootsvc->GetMemoryMap(&map->length, map->entries, &map->key, &map->size, &map->version);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to load memory map");
	return EFI_SUCCESS;
}

EFI_STATUS
memory_dump_map(struct memory_map *map)
{
	if (map == NULL)
		return EFI_INVALID_PARAMETER;

	const size_t count = map->length / map->size;

	console::print(L"Memory map:\n");
	console::print(L"\t	Descriptor Count: %d (%d bytes)\n", count, map->length);
	console::print(L"\t  Descriptor Size: %d bytes\n", map->size);
	console::print(L"\t      Type offset: %d\n\n", offsetof(EFI_MEMORY_DESCRIPTOR, Type));

	EFI_MEMORY_DESCRIPTOR *end = INCREMENT_DESC(map->entries, map->length);
	EFI_MEMORY_DESCRIPTOR *d = map->entries;
	while (d < end) {
		int runtime = (d->Attribute & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME;
		console::print(L"%s%s ", memory_type_name(d->Type), runtime ? L"*" : L" ");
		console::print(L"%lx ", d->PhysicalStart);
		console::print(L"%lx ", d->VirtualStart);
		console::print(L"[%4d]\n", d->NumberOfPages);

		d = INCREMENT_DESC(d, map->size);
	}

	return EFI_SUCCESS;
}

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

} // namespace boot
} // namespace memory
