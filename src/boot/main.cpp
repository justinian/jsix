#include <uefi/types.h>
#include <uefi/guid.h>
#include <uefi/tables.h>
#include <uefi/protos/simple_text_output.h>

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "error.h"
#include "memory.h"
/*
#include "guids.h"
#include "kernel_args.h"
#include "loader.h"
#include "utility.h"

#ifndef SCRATCH_PAGES
#define SCRATCH_PAGES 64
#endif

#define KERNEL_HEADER_MAGIC   0x600db007
#define KERNEL_HEADER_VERSION 1

#pragma pack(push, 1)
struct kernel_header {
	uint32_t magic;
	uint16_t version;
	uint16_t length;

	uint8_t major;
	uint8_t minor;
	uint16_t patch;
	uint32_t gitsha;
};
#pragma pack(pop)
using kernel_entry = void (*)(kernel_args *);
*/

namespace boot {


/*
static void
type_to_wchar(wchar_t *into, uint32_t type)
{
	for (int j=0; j<4; ++j)
		into[j] = static_cast<wchar_t>(reinterpret_cast<char *>(&type)[j]);
}

EFI_STATUS
detect_debug_mode(EFI_RUNTIME_SERVICES *run, kernel_args *header) {
	CHAR16 var_name[] = L"debug";

	EFI_STATUS status;
	uint8_t debug = 0;
	UINTN var_size = sizeof(debug);

#ifdef __JSIX_SET_DEBUG_UEFI_VAR__
	debug = __JSIX_SET_DEBUG_UEFI_VAR__;
	uint32_t attrs =
		EFI_VARIABLE_NON_VOLATILE |
		EFI_VARIABLE_BOOTSERVICE_ACCESS |
		EFI_VARIABLE_RUNTIME_ACCESS;
	status = run->SetVariable(
			var_name,
			&guid_jsix_vendor,
			attrs,
			var_size,
			&debug);
	CHECK_EFI_STATUS_OR_RETURN(status, "detect_debug_mode::SetVariable");
#endif

	status = run->GetVariable(
			var_name,
			&guid_jsix_vendor,
			nullptr,
			&var_size,
			&debug);
	CHECK_EFI_STATUS_OR_RETURN(status, "detect_debug_mode::GetVariable");

	if (debug)
		header->flags |= JSIX_FLAG_DEBUG;

	return EFI_SUCCESS;
}
*/

uefi::status
bootloader_main_uefi(uefi::system_table *st, console &con)
{
	error::uefi_handler handler(con);

	uefi::boot_services *bs = st->boot_services;
	uefi::runtime_services *rs = st->runtime_services;

	/*
	con.status_begin(L"Trying to do a harder thing...");
	con.status_warn(L"First warning");
	con.status_warn(L"Second warning");

	con.status_begin(L"Trying to do the impossible...");
	con.status_warn(L"we're not going to make it");

	error::raise(uefi::status::unsupported, L"OH NO");
	*/

	con.status_begin(L"Initializing pointer fixup for virtualization");
	memory::init_pointer_fixup(bs, rs);
	con.status_ok();

	// Find ACPI tables. Ignore ACPI 1.0 if a 2.0 table is found.
	//
	con.status_begin(L"Searching for ACPI table");
	uintptr_t acpi_table = 0;
	for (size_t i = 0; i < st->number_of_table_entries; ++i) {
		uefi::configuration_table *table = &st->configuration_table[i];

		if (table->vendor_guid == uefi::vendor_guids::acpi2) {
			acpi_table = reinterpret_cast<uintptr_t>(table->vendor_table);
			break;
		}
		
		if (table->vendor_guid == uefi::vendor_guids::acpi1) {
			// Mark a v1 table with the LSB high
			acpi_table = reinterpret_cast<uintptr_t>(table->vendor_table);
			acpi_table |= 1;
		}
	}

	if (!acpi_table) {
		error::raise(uefi::status::not_found, L"Could not find ACPI table");
	} else if (acpi_table & 1) {
		con.status_warn(L"Only found ACPI 1.0 table");
	} else {
		con.status_ok();
	}

	while(1);
	return uefi::status::success;
}
	/*

	// Compute necessary number of data pages
	//
	size_t data_length = 0;
	status = memory_get_map_length(bootsvc, &data_length);
	CHECK_EFI_STATUS_OR_FAIL(status);

	size_t header_size = sizeof(kernel_args);
	const size_t header_align = alignof(kernel_args);
	if (header_size % header_align)
		header_size += header_align - (header_size % header_align);

	data_length += header_size;

	// Load the kernel image from disk and check it
	//
	console::print(L"Loading kernel into memory...\r\n");

	struct loader_data load;
	load.data_length = data_length;
	status = loader_load_kernel(bootsvc, &load);
	CHECK_EFI_STATUS_OR_FAIL(status);

	console::print(L"    %x image bytes  at 0x%x\r\n", load.kernel_length, load.kernel);
	console::print(L"    %x data bytes   at 0x%x\r\n", load.data_length, load.data);
	console::print(L"    %x initrd bytes at 0x%x\r\n", load.initrd_length, load.initrd);

	struct kernel_header *version = (struct kernel_header *)load.kernel;
	if (version->magic != KERNEL_HEADER_MAGIC) {
		console::print(L"    bad magic %x\r\n", version->magic);
		CHECK_EFI_STATUS_OR_FAIL(EFI_CRC_ERROR);
	}

	console::print(L"    Kernel version %d.%d.%d %x%s\r\n",
			version->major, version->minor, version->patch, version->gitsha & 0x0fffffff,
			version->gitsha & 0xf0000000 ? L"*" : L"");
	console::print(L"    Entrypoint 0x%x\r\n", load.kernel_entry);

	kernel_entry kernel_main =
		reinterpret_cast<kernel_entry>(load.kernel_entry);
	memory_mark_pointer_fixup((void **)&kernel_main);

	// Set up the kernel data pages to pass to the kernel
	//
	struct kernel_args *data_header = (struct kernel_args *)load.data;
	memory_mark_pointer_fixup((void **)&data_header);

	data_header->magic = DATA_HEADER_MAGIC;
	data_header->version = DATA_HEADER_VERSION;
	data_header->length = sizeof(struct kernel_args);

	data_header->scratch_pages = SCRATCH_PAGES;
	data_header->flags = 0;

	data_header->initrd = load.initrd;
	data_header->initrd_length = load.initrd_length;
	memory_mark_pointer_fixup((void **)&data_header->initrd);

	data_header->data = load.data;
	data_header->data_length = load.data_length;
	memory_mark_pointer_fixup((void **)&data_header->data);

	data_header->memory_map = (EFI_MEMORY_DESCRIPTOR *)(data_header + 1);
	memory_mark_pointer_fixup((void **)&data_header->memory_map);

	data_header->runtime = runsvc;
	memory_mark_pointer_fixup((void **)&data_header->runtime);

	data_header->acpi_table = acpi_table;
	memory_mark_pointer_fixup((void **)&data_header->acpi_table);

	data_header->_reserved0 = 0;
	data_header->_reserved1 = 0;

	// Figure out the framebuffer (if any) and add that to the data header
	//
	status = con_get_framebuffer(
			bootsvc, 
			&data_header->frame_buffer,
			&data_header->frame_buffer_length,
			&data_header->hres,
			&data_header->vres,
			&data_header->rmask,
			&data_header->gmask,
			&data_header->bmask);
	CHECK_EFI_STATUS_OR_FAIL(status);
	memory_mark_pointer_fixup((void **)&data_header->frame_buffer);

	// Save the memory map and tell the firmware we're taking control.
	//
	struct memory_map map;
	map.length = (load.data_length - header_size);
	map.entries =
		reinterpret_cast<EFI_MEMORY_DESCRIPTOR *>(data_header->memory_map);

	status = memory_get_map(bootsvc, &map);
	CHECK_EFI_STATUS_OR_FAIL(status);

	data_header->memory_map_length = map.length;
	data_header->memory_map_desc_size = map.size;

	detect_debug_mode(runsvc, data_header);

	// bootsvc->Stall(5000000);

	status = bootsvc->ExitBootServices(image_handle, map.key);
	CHECK_EFI_STATUS_OR_ASSERT(status, 0);

	memory_virtualize(runsvc, &map);

	// Hand control to the kernel
	//
	kernel_main(data_header);
	return EFI_LOAD_ERROR;
}
*/
} // namespace boot

extern "C" uefi::status
efi_main(uefi::handle image_handle, uefi::system_table *st)
{
	using namespace boot;

	error::cpu_assert_handler handler;
	console con(st->boot_services, st->con_out);

	return bootloader_main_uefi(st, con);
}

