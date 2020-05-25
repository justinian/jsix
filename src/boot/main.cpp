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
#include "paging.h"

#include "kernel_args.h"

namespace kernel {
#include "kernel_memory.h"
}

namespace boot {

constexpr int max_modules = 10; // Max modules to allocate room for

/// Change a pointer to point to the higher-half linear-offset version
/// of the address it points to.
template <typename T>
void change_pointer(T *&pointer)
{
	pointer = offset_ptr<T>(pointer, kernel::memory::page_offset);
}

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
kernel::args::header *
bootloader_main_uefi(
	uefi::handle image,
	uefi::system_table *st,
	console &con,
	kernel::entrypoint *kentry)
{
	error::uefi_handler handler(con);
	status_line status(L"Performing UEFI pre-boot");

	uefi::boot_services *bs = st->boot_services;
	uefi::runtime_services *rs = st->runtime_services;
	memory::init_pointer_fixup(bs, rs);

	kernel::args::header *args =
		allocate_args_structure(bs, max_modules);

	args->magic = kernel::args::magic;
	args->version = kernel::args::version;
	args->runtime_services = rs;
	args->acpi_table = hw::find_acpi_table(st);

	memory::mark_pointer_fixup(&args->runtime_services);

	fs::file disk = fs::get_boot_volume(image, bs);
	load_module(disk, args, L"initrd", L"initrd.img", kernel::args::mod_type::initrd);

	kernel::args::module *kernel =
		load_module(disk, args, L"kernel", L"jsix.elf", kernel::args::mod_type::kernel);

	paging::allocate_tables(args, bs);
	*kentry = loader::load(kernel->location, kernel->size, args, bs);

	for (unsigned i = 0; i < args->num_modules; ++i) {
		kernel::args::module &mod = args->modules[i];
		change_pointer(mod.location);
	}

	return args;
}

} // namespace boot

/// The UEFI entrypoint for the loader.
extern "C" uefi::status
efi_main(uefi::handle image_handle, uefi::system_table *st)
{
	using namespace boot;

	error::cpu_assert_handler handler;
	console con(st->boot_services, st->con_out);

	kernel::entrypoint kentry = nullptr;
	kernel::args::header *args =
		bootloader_main_uefi(image_handle, st, con, &kentry);

	memory::efi_mem_map map =
		memory::build_kernel_mem_map(args, st->boot_services);

	try_or_raise(
		st->boot_services->exit_boot_services(image_handle, map.key),
		L"Failed to exit boot services");

	memory::virtualize(args->pml4, map, st->runtime_services);
	change_pointer(args->pml4);
	hw::setup_cr4();

	kentry(args);
	debug_break();
	return uefi::status::unsupported;
}

