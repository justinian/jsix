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
struct kernel_header {
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

int is_guid(EFI_GUID *a, EFI_GUID *b)
{
	uint64_t *ai = (uint64_t *)a;
	uint64_t *bi = (uint64_t *)b;
	return ai[0] == bi[0] && ai[1] == bi[1];
}

EFI_STATUS
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
	EFI_STATUS status;
	EFI_BOOT_SERVICES *bootsvc = system_table->BootServices;

	// When checking the console initialization error, use
	// CHECK_EFI_STATUS_OR_RETURN because we can't be sure
	// if the console was fully set up
	status = con_initialize(system_table, GIT_VERSION);
	CHECK_EFI_STATUS_OR_RETURN(status, "con_initialize");

	// From here on out, use CHECK_EFI_STATUS_OR_FAIL instead
	// because the console is now set up

	EFI_GUID acpi1_guid = ACPI_TABLE_GUID;
	EFI_GUID acpi2_guid = {0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81}};

	con_status_begin(L"Reading configuration tables...");
	con_printf(L" %d found.\n", system_table->NumberOfTableEntries); 

	for (size_t i=0; i<system_table->NumberOfTableEntries; ++i) {
		EFI_CONFIGURATION_TABLE *efi_table = &system_table->ConfigurationTable[i];

		con_printf(L"    %d. ", i);

		if (is_guid(&efi_table->VendorGuid, &acpi2_guid)) {
			char *table = efi_table->VendorTable;
			con_printf(L" ACPI 2.0 Table.");
		} else if (is_guid(&efi_table->VendorGuid, &acpi1_guid)) {
			char *table = efi_table->VendorTable;
			con_printf(L" ACPI 1.0 Table.");
		/*
		} else if (is_guid(&efi_table->VendorGuid, &MpsTableGuid)) {
			con_printf(L" MPS.");
		} else if (is_guid(&efi_table->VendorGuid, &SMBIOSTableGuid)) {
			con_printf(L" SMBIOS.");
		} else if (is_guid(&efi_table->VendorGuid, &SalSystemTableGuid)) {
			con_printf(L" SAL.");
		*/
		} else {
			con_printf(L" Other.");
		}

		con_printf(L"\n");
	}
	con_status_ok();


	con_status_begin(L"Computing needed data pages...");
	UINTN data_length = 0;
	status = memory_get_map_length(bootsvc, &data_length);

	size_t header_size = sizeof(struct popcorn_data);
	const size_t header_align = alignof(struct popcorn_data);
	if (header_size % header_align)
		header_size += header_align - (header_size % header_align);

	data_length += header_size;
	con_status_ok();

	con_status_begin(L"Looking for devices...\n");
	EFI_GUID serial_guid = EFI_SERIAL_IO_PROTOCOL_GUID;
	size_t num_serial = 0;
	EFI_HANDLE *serial_handles;
	status = bootsvc->LocateHandleBuffer(ByProtocol, &serial_guid, NULL, &num_serial, &serial_handles);
	CHECK_EFI_STATUS_OR_FAIL(status);
	con_printf(L"    %d found.\n", num_serial);

	EFI_GUID ptt_guid = EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID;
	EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *ptt = NULL;
	status = bootsvc->LocateProtocol(&ptt_guid, NULL, (void **)&ptt);
	CHECK_EFI_STATUS_OR_FAIL(status);

	EFI_GUID path_guid = EFI_DEVICE_PATH_PROTOCOL_GUID;
	for (size_t i=0; i<num_serial; ++i) {
		EFI_HANDLE handle = serial_handles[i];
		EFI_DEVICE_PATH_PROTOCOL *path;
		status = bootsvc->HandleProtocol(handle, &path_guid, (void **)&path);
		CHECK_EFI_STATUS_OR_FAIL(status);
		con_printf(L"    %s\n", ptt->ConvertDevicePathToText(path, 1, 0));
	}

	con_status_ok();

	con_status_begin(L"Loading kernel into memory...");
	void *kernel_image = NULL, *kernel_data = NULL;
	uint64_t kernel_length = 0;
	status = loader_load_kernel(bootsvc, &kernel_image, &kernel_length, &kernel_data, &data_length);
	CHECK_EFI_STATUS_OR_FAIL(status);
	con_printf(L"\n    %u bytes at 0x%x", kernel_length, kernel_image);
	con_printf(L"\n    %u data bytes at 0x%x", data_length, kernel_data);

	struct kernel_header *version = (struct kernel_header *)kernel_image;
	if (version->magic != KERNEL_HEADER_MAGIC) {
		con_printf(L"\n    bad magic %x", version->magic);
		CHECK_EFI_STATUS_OR_FAIL(EFI_CRC_ERROR);
	}

	con_printf(L"\n    Kernel version %d.%d.%d %x%s", version->major, version->minor, version->patch,
		  version->gitsha & 0x0fffffff, version->gitsha & 0xf0000000 ? "*" : "");
	con_printf(L"\n    Entrypoint 0x%x", version->entrypoint);

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
	data_header->runtime = system_table->RuntimeServices;

	con_status_begin(L"Exiting boot services...");

	struct memory_map map;
	map.entries = data_header->memory_map;
	map.length = (data_length - header_size);

	status = memory_get_map(bootsvc, &map);
	CHECK_EFI_STATUS_OR_FAIL(status);

	status = bootsvc->ExitBootServices(image_handle, map.key);
	CHECK_EFI_STATUS_OR_ASSERT(status, 0);

	kernel_main(data_header);
	return EFI_LOAD_ERROR;
}
