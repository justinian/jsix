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

void EFIAPI
memory_update_addresses(EFI_EVENT UNUSED *event, void *context)
{
	EFI_STATUS status;
	status = ST->RuntimeServices->ConvertPointer(0, (void **)context);

	CHECK_EFI_STATUS_OR_ASSERT(status, *((void **)context));
}

EFI_STATUS
memory_mark_address_for_update(void **pointer)
{
	EFI_EVENT event;
	EFI_STATUS status;

	status = ST->BootServices->CreateEvent(EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE, TPL_CALLBACK,
										   (EFI_EVENT_NOTIFY)&memory_update_addresses,
										   (void *)pointer, &event);

	CHECK_EFI_STATUS_OR_ASSERT(status, pointer);
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
memory_copy_map(EFI_MEMORY_DESCRIPTOR *oldmap, EFI_MEMORY_DESCRIPTOR *newmap, size_t oldmap_size,
				size_t desc_size, void *this_image, size_t *newmap_size)
{
	size_t count = 0;

	EFI_MEMORY_DESCRIPTOR *s;
	EFI_MEMORY_DESCRIPTOR *end = INCREMENT_DESC(oldmap, oldmap_size);
	for (s = oldmap; s < end; s = INCREMENT_DESC(s, desc_size)) {
		if (this_image != (void *)s->PhysicalStart &&
			s->Type == EfiLoaderCode ||
			s->Type == EfiLoaderData ||
			s->Type == EfiBootServicesCode ||
			s->Type == EfiBootServicesData ||
			s->Type == EfiConventionalMemory) {
			// These are memory types we don't need to keep
			continue;
		}

		s->Attribute |= EFI_MEMORY_RUNTIME;
		s->VirtualStart = (EFI_VIRTUAL_ADDRESS)(s->PhysicalStart + VIRTUAL_OFFSET);

		EFI_MEMORY_DESCRIPTOR *d = INCREMENT_DESC(newmap, count*desc_size);
		copy_desc(s, d, desc_size);
		++count;
	}

	*newmap_size = desc_size * count;
	return EFI_SUCCESS;
}

EFI_STATUS
memory_get_map_size(size_t *size)
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
memory_get_map(EFI_MEMORY_DESCRIPTOR **buffer, size_t *buffer_size, size_t *key, size_t *desc_size,
			   UINT32 *desc_version)
{
	EFI_STATUS status;

	size_t needs_size = 0;
	status = ST->BootServices->GetMemoryMap(&needs_size, 0, key, desc_size, desc_version);
	if (status != EFI_BUFFER_TOO_SMALL) {
		CHECK_EFI_STATUS_OR_RETURN(status, "Failed to load memory map");
	}

	// Give some extra buffer to account for changes.
	*buffer_size = needs_size + 256;
	status = ST->BootServices->AllocatePool(EfiLoaderData, *buffer_size, (void **)buffer);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to allocate space for memory map");

	status = ST->BootServices->GetMemoryMap(buffer_size, *buffer, key, desc_size, desc_version);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to load memory map");
	return EFI_SUCCESS;
}

EFI_STATUS
memory_dump_map(EFI_MEMORY_DESCRIPTOR *memory_map, size_t memmap_size, size_t desc_size)
{
	const size_t count = memmap_size / desc_size;

	Print(L"Memory map:\n");
	Print(L"\t	Descriptor Count: %d (%d bytes)\n", count, memmap_size);
	Print(L"\t   Descriptor Size: %d bytes\n", desc_size);
	Print(L"\t       Type offset: %d\n", offsetof(EFI_MEMORY_DESCRIPTOR, Type));
	Print(L"\t   Physical offset: %d\n", offsetof(EFI_MEMORY_DESCRIPTOR, PhysicalStart));
	Print(L"\t    Virtual offset: %d\n", offsetof(EFI_MEMORY_DESCRIPTOR, VirtualStart));
	Print(L"\t      Pages offset: %d\n", offsetof(EFI_MEMORY_DESCRIPTOR, NumberOfPages));
	Print(L"\t       Attr offset: %d\n\n", offsetof(EFI_MEMORY_DESCRIPTOR, Attribute));

	EFI_MEMORY_DESCRIPTOR *end = INCREMENT_DESC(memory_map, memmap_size);
	EFI_MEMORY_DESCRIPTOR *d = memory_map;
	while (d < end) {
		int runtime = (d->Attribute & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME;
		Print(L"%23s%s ", memory_type_name(d->Type), runtime ? L"*" : L" ");
		Print(L"%016llx ", d->PhysicalStart);
		Print(L"%016llx ", d->VirtualStart);
		Print(L"[%4d]\n", d->NumberOfPages);

		d = INCREMENT_DESC(d, desc_size);
	}

	return EFI_SUCCESS;
}
