#include <efi.h>
#include <efilib.h>
#include <stddef.h>

#include "loader.h"
#include "memory.h"
#include "utility.h"

#define INCREMENT_DESC(p, b)  (EFI_MEMORY_DESCRIPTOR*)(((uint8_t*)(p))+(b))

const size_t PAGE_SIZE = 4096;

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
memory_get_map_length(size_t *size)
{
	if (size == NULL)
		return EFI_INVALID_PARAMETER;

	EFI_STATUS status;
	size_t key, desc_size;
	uint32_t desc_version;
	*size = 0;
	status = ST->BootServices->GetMemoryMap(size, 0, &key, &desc_size, &desc_version);
	if (status != EFI_BUFFER_TOO_SMALL) {
		CHECK_EFI_STATUS_OR_RETURN(status, "Failed to get memory map size");
	}
	return EFI_SUCCESS;
}

EFI_STATUS
memory_get_map(struct memory_map *map)
{
	EFI_STATUS status;

	if (map == NULL)
		return EFI_INVALID_PARAMETER;

	size_t needs_size = 0;
	status = memory_get_map_length(&needs_size);
	if (EFI_ERROR(status)) return status;

	if (map->length < needs_size)
		return EFI_BUFFER_TOO_SMALL;

	status = ST->BootServices->GetMemoryMap(&map->length, map->entries, &map->key, &map->size, &map->version);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to load memory map");
	return EFI_SUCCESS;
}

EFI_STATUS
memory_dump_map(struct memory_map *map)
{
	if (map == NULL)
		return EFI_INVALID_PARAMETER;

	const size_t count = map->length / map->size;

	Print(L"Memory map:\n");
	Print(L"\t	Descriptor Count: %d (%d bytes)\n", count, map->length);
	Print(L"\t   Descriptor Size: %d bytes\n", map->size);
	Print(L"\t       Type offset: %d\n\n", offsetof(EFI_MEMORY_DESCRIPTOR, Type));

	EFI_MEMORY_DESCRIPTOR *end = INCREMENT_DESC(map->entries, map->length);
	EFI_MEMORY_DESCRIPTOR *d = map->entries;
	while (d < end) {
		int runtime = (d->Attribute & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME;
		Print(L"%23s%s ", memory_type_name(d->Type), runtime ? L"*" : L" ");
		Print(L"%016llx ", d->PhysicalStart);
		Print(L"%016llx ", d->VirtualStart);
		Print(L"[%4d]\n", d->NumberOfPages);

		d = INCREMENT_DESC(d, map->size);
	}

	return EFI_SUCCESS;
}
