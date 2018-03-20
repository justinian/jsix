#include <efi.h>
#include <efilib.h>

#include "console.h"
#include "loader.h"
#include "memory.h"
#include "utility.h"

#ifndef GIT_VERSION
	#define GIT_VERSION L"no version"
#endif

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS status;

	InitializeLib(ImageHandle, SystemTable);

	// When checking the console initialization error,
	// use CHECK_EFI_STATUS_OR_RETURN because we can't
	// be sure if the console was fully set up
	status = con_initialize(GIT_VERSION);
	CHECK_EFI_STATUS_OR_RETURN(status, "con_initialize");
	// From here on out, use CHECK_EFI_STATUS_OR_FAIL instead
	// because the console is now set up

	// Get info about the image
	con_status_begin(L"Gathering image information...");
	EFI_LOADED_IMAGE *info = 0;
	EFI_GUID image_proto = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	status = ST->BootServices->HandleProtocol(ImageHandle, &image_proto, (void **)&info);
    CHECK_EFI_STATUS_OR_FAIL(status);
	con_status_ok();

	// memory_dump_map();

    con_status_begin(L"Loading kernel into memory...");
    void *kernel_image = NULL;
	uint64_t kernel_length = 0;
    status = loader_load_kernel(&kernel_image, &kernel_length);
    CHECK_EFI_STATUS_OR_FAIL(status);
	Print(L" %u bytes at 0x%x", kernel_length, kernel_image);
    con_status_ok();

    void (*kernel_main)() = kernel_image;
    kernel_main();

    /*
	con_status_begin(L"Virtualizing memory...");
    status = memory_virtualize();
    CHECK_EFI_STATUS_OR_FAIL(status);
	con_status_ok();

	UINTN memmap_size = 0;
	EFI_MEMORY_DESCRIPTOR *memmap;
	UINTN memmap_key;
	UINTN desc_size;
	UINT32 desc_version;

	con_status_begin(L"Exiting boot services");
	memmap = LibMemoryMap(&memmap_size, &memmap_key,
		&desc_size, &desc_version);

	status = ST->BootServices->ExitBootServices(ImageHandle, memmap_key);
	CHECK_EFI_STATUS_OR_FAIL(status);
	con_status_ok();

	con_status_begin(L"Setting virtual address map");
	status = ST->RuntimeServices->SetVirtualAddressMap(
		memmap_size, desc_size, desc_version, memmap);
	CHECK_EFI_STATUS_OR_FAIL(status);
	con_status_ok();
    */

	while (1) __asm__("hlt");
	return status;
}

