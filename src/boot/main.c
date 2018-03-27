#include <efi.h>
#include <efilib.h>
#include <stdalign.h>
#include <stddef.h>

#include "console.h"
#include "loader.h"
#include "memory.h"
#include "utility.h"

#ifndef GIT_VERSION
#define GIT_VERSION L"no version"
#endif

#define KERNEL_HEADER_MAGIC   0x600db007
#define KERNEL_HEADER_VERSION 1

#define DATA_HEADER_MAGIC     0x600dda7a
#define DATA_HEADER_VERSION   1


#pragma pack(push, 1)
struct kernel_version {
	uint32_t magic;
	uint16_t version;
	uint16_t length;

	uint8_t major;
	uint8_t minor;
	uint16_t patch;
	uint32_t gitsha;

	void *entrypoint;
};

struct popcorn_data {
	uint32_t magic;
	uint16_t version;
	uint16_t length;

	uint16_t data_pages;
	uint16_t _reserverd;
	uint32_t flags;

	EFI_MEMORY_DESCRIPTOR *memory_map;
	EFI_RUNTIME_SERVICES *runtime;
}
__attribute__((aligned(_Alignof(EFI_MEMORY_DESCRIPTOR))));
#pragma pack(pop)


EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS status;

	UINTN memmap_size = 0;
	UINTN memmap_key = 0;
	UINTN desc_size = 0;
	UINTN newmap_size = 0;
	UINTN data_length = 0;

	UINT32 desc_version = 0;


	InitializeLib(ImageHandle, SystemTable);

	// When checking the console initialization error,
	// use CHECK_EFI_STATUS_OR_RETURN because we can't
	// be sure if the console was fully set up
	status = con_initialize(GIT_VERSION);
	CHECK_EFI_STATUS_OR_RETURN(status, "con_initialize");
	// From here on out, use CHECK_EFI_STATUS_OR_FAIL instead
	// because the console is now set up

	con_status_begin(L"Computing needed data pages...");
	status = memory_get_map_size(&data_length);
	size_t header_size = sizeof(struct popcorn_data);
	const size_t header_align = alignof(struct popcorn_data);
	if (header_size % header_align)
		header_size += header_align - (header_size % header_align);
	data_length += header_size;
	con_status_ok();

	con_status_begin(L"Loading kernel into memory...");
	void *kernel_image = NULL, *kernel_data = NULL;
	uint64_t kernel_length = 0;
	status = loader_load_kernel(&kernel_image, &kernel_length, &kernel_data, &data_length);
	CHECK_EFI_STATUS_OR_FAIL(status);
	Print(L"\n    %u bytes at 0x%x", kernel_length, kernel_image);
	Print(L"\n    %u data bytes at 0x%x", data_length, kernel_data);

	struct kernel_version *version = (struct kernel_version *)kernel_image;
	if (version->magic != KERNEL_HEADER_MAGIC) {
		Print(L"\n    bad magic %x", version->magic);
		CHECK_EFI_STATUS_OR_FAIL(EFI_CRC_ERROR);
	}

	Print(L"\n    Kernel version %d.%d.%d %x%s", version->major, version->minor, version->patch,
		  version->gitsha & 0x0fffffff, version->gitsha & 0xf0000000 ? "*" : "");
	Print(L"\n    Entrypoint 0x%x", version->entrypoint);

	void (*kernel_main)() = version->entrypoint;

	con_status_ok();

	struct popcorn_data *data_header = (struct popcorn_data *)kernel_data; 
	data_header->magic = DATA_HEADER_MAGIC;
	data_header->version = DATA_HEADER_VERSION;
	data_header->length = sizeof(struct popcorn_data);
	data_header->data_pages = data_length / 0x1000;
	data_header->_reserverd = 0;
	data_header->flags = 0;
	data_header->memory_map = (EFI_MEMORY_DESCRIPTOR *)(data_header + 1);
	data_header->runtime = SystemTable->RuntimeServices;

	// Get info about this image
	con_status_begin(L"Gathering image information...");
	EFI_LOADED_IMAGE *info = 0;
	EFI_GUID image_proto = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	status = ST->BootServices->HandleProtocol(ImageHandle, &image_proto, (void **)&info);
	CHECK_EFI_STATUS_OR_FAIL(status);
	con_status_ok();

	EFI_MEMORY_DESCRIPTOR *memory_map;

	/*
	memory_get_map(&memory_map, &memmap_size, &memmap_key, &desc_size, &desc_version);
	memory_dump_map(memory_map, memmap_size, desc_size);
	*/

	con_status_begin(L"Exiting boot services...");

	status = memory_mark_address_for_update((void **)&ST);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_mark_address_for_update((void **)&kernel_image);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_mark_address_for_update((void **)&kernel_main);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_mark_address_for_update((void **)&kernel_data);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_get_map(&memory_map, &memmap_size, &memmap_key, &desc_size, &desc_version);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = memory_copy_map(memory_map, data_header->memory_map, memmap_size, desc_size, info->ImageBase, &newmap_size);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = ST->BootServices->ExitBootServices(ImageHandle, memmap_key);
	CHECK_EFI_STATUS_OR_ASSERT(status, 0);

	kernel_main();
	return EFI_LOAD_ERROR;
}
