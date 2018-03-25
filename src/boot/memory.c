#include <efi.h>
#include <efilib.h>

#include "memory.h"
#include "loader.h"
#include "utility.h"

const UINTN PAGE_SIZE = 4096;

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

static const CHAR16 *memory_type_name(UINT32 value) {
	if (value >= (sizeof(memory_type_names)/sizeof(CHAR16*))) {
		if (value == KERNEL_MEMTYPE)
			return L"Kernel Image";
		return L"Bad Type Value";
	}
	return memory_type_names[value];
}

void EFIAPI memory_update_addresses(EFI_EVENT UNUSED *event, void *context) {
	EFI_STATUS status;
	status = ST->RuntimeServices->ConvertPointer(0, (void **)context);

	CHECK_EFI_STATUS_OR_ASSERT(status, *((void**)context));
}

EFI_STATUS memory_mark_address_for_update(void **pointer) {
	EFI_EVENT event;
	EFI_STATUS status;

	status = ST->BootServices->CreateEvent(
			EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
			TPL_CALLBACK,
			(EFI_EVENT_NOTIFY)&memory_update_addresses,
			(void*)pointer,
			&event);

	CHECK_EFI_STATUS_OR_ASSERT(status, pointer);
}

EFI_STATUS
memory_virtualize(
		EFI_MEMORY_DESCRIPTOR *memory_map,
		UINTN memmap_size,
		UINTN desc_size,
		UINT32 desc_version) {

	EFI_MEMORY_DESCRIPTOR *end = (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)memory_map + memmap_size);
	EFI_MEMORY_DESCRIPTOR *d = memory_map;
	while (d < end) {
		if (d->Type == KERNEL_MEMTYPE) {
			//d->VirtualStart = (EFI_VIRTUAL_ADDRESS)KERNEL_VIRT_ADDRESS;
			d->VirtualStart = (EFI_VIRTUAL_ADDRESS)d->PhysicalStart;
			d->Attribute |= EFI_MEMORY_RUNTIME;
		}
		else /*if (d->Attribute & EFI_MEMORY_RUNTIME)*/ {
			d->VirtualStart = (EFI_VIRTUAL_ADDRESS)d->PhysicalStart;
		}

		d = (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)d + desc_size);
	}

	return ST->RuntimeServices->SetVirtualAddressMap(memmap_size, desc_size, desc_version, memory_map);
}

EFI_STATUS memory_get_map(EFI_MEMORY_DESCRIPTOR **buffer, UINTN *buffer_size,
						  UINTN *key, UINTN *desc_size, UINT32 *desc_version) {
	EFI_STATUS status;

	UINTN needs_size = 0;
	status = ST->BootServices->GetMemoryMap(&needs_size, 0, key, desc_size, desc_version);
	if (status != EFI_BUFFER_TOO_SMALL) {
		CHECK_EFI_STATUS_OR_RETURN(status, "Failed to load memory map");
	}

	// Give some extra buffer to account for changes.
	*buffer_size = needs_size + 256;
	status = ST->BootServices->AllocatePool(EfiLoaderData, *buffer_size, (void**)buffer);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to allocate space for memory map");

	status = ST->BootServices->GetMemoryMap(buffer_size, *buffer, key, desc_size, desc_version);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to load memory map");
	return EFI_SUCCESS;
}

EFI_STATUS memory_dump_map() {
	EFI_MEMORY_DESCRIPTOR *buffer;
	UINTN buffer_size, desc_size, key;
	UINT32 desc_version;

	EFI_STATUS status = memory_get_map(&buffer, &buffer_size, &key,
		&desc_size, &desc_version);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to get memory map");

	const UINTN count = buffer_size / desc_size;

	Print(L"Memory map:\n");
	Print(L"\t	Descriptor Count: %d (%d bytes)\n", count, buffer_size);
	Print(L"\t		 Version Key: %d\n", key);
	Print(L"\tDescriptor Version: %d (%d bytes)\n\n", desc_version, desc_size);

	EFI_MEMORY_DESCRIPTOR *end = (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)buffer + buffer_size);
	EFI_MEMORY_DESCRIPTOR *d = buffer;
	while (d < end) {
		int runtime = (d->Attribute & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME;
		Print(L"%23s%s ", memory_type_name(d->Type), runtime ? L"*" : L" ");
		Print(L"%016llx (%3d pages)\n", d->PhysicalStart, d->NumberOfPages);

		d = (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)d + desc_size);
	}

	ST->BootServices->FreePool(buffer);
	return EFI_SUCCESS;
}
