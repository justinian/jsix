#include <efi.h>
#include <efilib.h>

#include "console.h"
#include "loader.h"
#include "memory.h"
#include "utility.h"

#ifndef GIT_VERSION
#define GIT_VERSION L"no version"
#endif

#define KERNEL_MAGIC 0x600db007

#pragma pack(push, 1)
struct kernel_version {
	uint32_t magic;
	uint8_t major;
	uint8_t minor;
	uint16_t patch;
	uint32_t gitsha;
	void *entrypoint;
};
#pragma pack(pop)

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
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

	con_status_begin(L"Loading kernel into memory...");
	void *kernel_image = NULL;
	uint64_t kernel_length = 0;
	status = loader_load_kernel(&kernel_image, &kernel_length);
	CHECK_EFI_STATUS_OR_FAIL(status);
	Print(L"\n    %u bytes at 0x%x", kernel_length, kernel_image);

	struct kernel_version *version = (struct kernel_version *)kernel_image;
	if (version->magic != KERNEL_MAGIC) {
		Print(L"\n    bad magic %x", version->magic);
		CHECK_EFI_STATUS_OR_FAIL(EFI_CRC_ERROR);
	}

	Print(L"\n    Kernel version %d.%d.%d %x%s", version->major, version->minor, version->patch,
		  version->gitsha & 0x0fffffff, version->gitsha & 0xf0000000 ? "*" : "");
	Print(L"\n    Entrypoint %d", version->entrypoint);

	void (*kernel_main)() = version->entrypoint;
	con_status_ok();

	// memory_dump_map();

	con_status_begin(L"Exiting boot services...");
	UINTN memmap_size = 0, memmap_key = 0;
	UINTN desc_size = 0;
	UINT32 desc_version = 0;
	EFI_MEMORY_DESCRIPTOR *memory_map;

	status = memory_mark_address_for_update((void **)&ST);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_mark_address_for_update((void **)&kernel_image);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_mark_address_for_update((void **)&kernel_main);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_get_map(&memory_map, &memmap_size, &memmap_key, &desc_size, &desc_version);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = ST->BootServices->ExitBootServices(ImageHandle, memmap_key);
	CHECK_EFI_STATUS_OR_ASSERT(status, 0);

	status = memory_virtualize(memory_map, memmap_size, desc_size, desc_version);
	CHECK_EFI_STATUS_OR_ASSERT(status, 0);

	kernel_main();
	return EFI_LOAD_ERROR;
}
