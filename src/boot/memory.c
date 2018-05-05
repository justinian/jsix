#include <stddef.h>

#include "loader.h"
#include "memory.h"
#include "utility.h"

#define INCREMENT_DESC(p, b)  (EFI_MEMORY_DESCRIPTOR*)(((uint8_t*)(p))+(b))

size_t fixup_pointer_index = 0;
void **fixup_pointers[64];
uint64_t *new_pml4 = 0;

const CHAR16 *memory_type_names[] = {
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

static const CHAR16 *
memory_type_name(UINT32 value)
{
	if (value >= (sizeof(memory_type_names) / sizeof(CHAR16 *))) {
		if (value == KERNEL_DATA_MEMTYPE) return L"Kernel Data";
		else if (value == KERNEL_MEMTYPE) return L"Kernel Image";
		else return L"Bad Type Value";
	}
	return memory_type_names[value];
}

void EFIAPI
memory_update_marked_addresses(EFI_EVENT UNUSED *event, void *context)
{
	EFI_RUNTIME_SERVICES *runsvc = (EFI_RUNTIME_SERVICES*)context;
	for (size_t i = 0; i < fixup_pointer_index; ++i) {
		if (fixup_pointers[i])
			runsvc->ConvertPointer(0, fixup_pointers[i]);
	}
}

EFI_STATUS
memory_init_pointer_fixup(EFI_BOOT_SERVICES *bootsvc, EFI_RUNTIME_SERVICES *runsvc)
{
	EFI_STATUS status;
	EFI_EVENT event;

	status = bootsvc->CreateEvent(
			EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
			TPL_CALLBACK,
			(EFI_EVENT_NOTIFY)&memory_update_marked_addresses,
			runsvc,
			&event);

	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to initialize pointer update event.");

	// Reserve a page for our replacement PML4, plus some pages for the kernel to use
	// as page tables while it gets started.
	EFI_PHYSICAL_ADDRESS addr = 0;
	status = bootsvc->AllocatePages(AllocateAnyPages, EfiLoaderData, 16, &addr);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to allocate page table pages.");

	new_pml4 = (uint64_t *)addr;
	return EFI_SUCCESS;
}

void
memory_mark_pointer_fixup(void **p)
{
	if (fixup_pointer_index == 0) {
		const size_t count = sizeof(fixup_pointers) / sizeof(void*);
		for (size_t i = 0; i < count; ++i) fixup_pointers[i] = 0;
	}
	fixup_pointers[fixup_pointer_index++] = p;
}

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

	con_printf(L"Memory map:\n");
	con_printf(L"\t	Descriptor Count: %d (%d bytes)\n", count, map->length);
	con_printf(L"\t  Descriptor Size: %d bytes\n", map->size);
	con_printf(L"\t      Type offset: %d\n\n", offsetof(EFI_MEMORY_DESCRIPTOR, Type));

	EFI_MEMORY_DESCRIPTOR *end = INCREMENT_DESC(map->entries, map->length);
	EFI_MEMORY_DESCRIPTOR *d = map->entries;
	while (d < end) {
		int runtime = (d->Attribute & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME;
		con_printf(L"%s%s ", memory_type_name(d->Type), runtime ? L"*" : L" ");
		con_printf(L"%lx ", d->PhysicalStart);
		con_printf(L"%lx ", d->VirtualStart);
		con_printf(L"[%4d]\n", d->NumberOfPages);

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
		case KERNEL_MEMTYPE:
		case KERNEL_FONT_MEMTYPE:
		case KERNEL_DATA_MEMTYPE:
			d->Attribute |= EFI_MEMORY_RUNTIME;

		default:
			if (d->Attribute & EFI_MEMORY_RUNTIME) {
				d->VirtualStart = d->PhysicalStart + KERNEL_VIRT_ADDRESS;
			}
		}
		d = INCREMENT_DESC(d, map->size);
	}

	runsvc->SetVirtualAddressMap(map->length, map->size, map->version, map->entries);
}
