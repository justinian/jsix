#include <uefi/types.h>
#include <uefi/guid.h>
#include <uefi/tables.h>
#include <uefi/protos/simple_text_output.h>

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "error.h"
#include "fs.h"
#include "hardware.h"
#include "loader.h"
#include "memory.h"

#include "kernel_args.h"
/*
#include "guids.h"
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
*/

namespace boot {

constexpr int max_modules = 10; // Max modules to allocate room for

/*

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

/// Allocate space for kernel args. Allocates enough space from pool
/// memory for the args header and `max_modules` module headers.
kernel::args::header *
allocate_args_structure(
	uefi::boot_services *bs,
	size_t max_modules)
{
	status_line status(L"Setting up kernel args memory");

	kernel::args::header *args = nullptr;

	size_t args_size =
		sizeof(kernel::args::header) + // The header itself
		max_modules * sizeof(kernel::args::module); // The module structures

	try_or_raise(
		bs->allocate_pool(memory::args_type, args_size,
			reinterpret_cast<void**>(&args)),
		L"Could not allocate argument memory");

	bs->set_mem(args, args_size, 0);

	args->modules =
		reinterpret_cast<kernel::args::module*>(args + 1);
	args->num_modules = 0;

	return args;
}

/// Load a file from disk into memory. Also adds an entry to the kernel
/// args module headers pointing at the loaded data.
/// \arg disk  The opened UEFI filesystem to load from
/// \arg args  The kernel args header to update with module information
/// \arg name  Name of the module (informational only)
/// \arg path  Path on `disk` of the file to load
/// \arg type  Type specifier of this module (eg, initrd or kernel)
kernel::args::module *
load_module(
	fs::file &disk,
	kernel::args::header *args,
	const wchar_t *name,
	const wchar_t *path,
	kernel::args::mod_type type)
{
	status_line status(L"Loading module", name);

	fs::file file = disk.open(path);
	kernel::args::module &module = args->modules[args->num_modules++];
	module.type = type;
	module.location = file.load(&module.size, memory::module_type);

	console::print(L"    Loaded at: 0x%lx, %d bytes\r\n", module.location, module.size);
	return &module;
}

/// The main procedure for the portion of the loader that runs while
/// UEFI is still in control of the machine. (ie, while the loader still
/// has access to boot services.
loader::loaded_elf
bootloader_main_uefi(uefi::handle image, uefi::system_table *st, console &con, size_t *map_key)
{
	error::uefi_handler handler(con);

	uefi::boot_services *bs = st->boot_services;
	uefi::runtime_services *rs = st->runtime_services;

	memory::init_pointer_fixup(bs, rs);

	kernel::args::header *args =
		allocate_args_structure(bs, max_modules);

	args->magic = kernel::args::magic;
	args->version = kernel::args::version;
	args->runtime_services = rs;
	args->acpi_table = hw::find_acpi_table(st);

	fs::file disk = fs::get_boot_volume(image, bs);
	load_module(disk, args, L"initrd", L"initrd.img", kernel::args::mod_type::initrd);

	kernel::args::module *kernel =
		load_module(disk, args, L"kernel", L"jsix.elf", kernel::args::mod_type::kernel);


	loader::loaded_elf kernel_elf =
		loader::load(kernel->location, kernel->size, bs);

	memory::efi_mem_map efi_map = memory::get_uefi_mappings(bs);
	memory::build_kernel_mem_map(efi_map, args, bs);

	efi_map = memory::get_uefi_mappings(bs);
	*map_key = efi_map.key;

	return kernel_elf;
}

	/*

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

/// The UEFI entrypoint for the loader.
extern "C" uefi::status
efi_main(uefi::handle image_handle, uefi::system_table *st)
{
	using namespace boot;

	error::cpu_assert_handler handler;
	console con(st->boot_services, st->con_out);

	size_t map_key;
	loader::loaded_elf kernel =
		bootloader_main_uefi(image_handle, st, con, &map_key);

	try_or_raise(
		st->boot_services->exit_boot_services(image_handle, map_key),
		L"Failed to exit boot services");

	debug_break();
	return uefi::status::unsupported;
}

