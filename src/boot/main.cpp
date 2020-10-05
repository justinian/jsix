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

namespace args = kernel::args;

namespace boot {

constexpr int max_modules = 5; // Max modules to allocate room for
constexpr int max_programs = 5; // Max programs to allocate room for

struct program_desc
{
	const wchar_t *name;
	const wchar_t *path;
};

const program_desc program_list[] = {
	{L"kernel", L"jsix.elf"},
	{L"null driver", L"nulldrv.elf"},
	//{L"terminal driver", L"terminal.elf"},
};

/// Change a pointer to point to the higher-half linear-offset version
/// of the address it points to.
template <typename T>
void change_pointer(T *&pointer)
{
	pointer = offset_ptr<T>(pointer, kernel::memory::page_offset);
}

/// Allocate space for kernel args. Allocates enough space from pool
/// memory for the args header and the module and program headers.
args::header *
allocate_args_structure(
	uefi::boot_services *bs,
	size_t max_modules,
	size_t max_programs)
{
	status_line status(L"Setting up kernel args memory");

	args::header *args = nullptr;

	size_t args_size =
		sizeof(args::header) + // The header itself
		max_modules * sizeof(args::module) +  // The module structures
		max_programs * sizeof(args::program); // The program structures

	try_or_raise(
		bs->allocate_pool(memory::args_type, args_size,
			reinterpret_cast<void**>(&args)),
		L"Could not allocate argument memory");

	bs->set_mem(args, args_size, 0);

	args->modules =
		reinterpret_cast<args::module*>(args + 1);
	args->num_modules = 0;

	args->programs =
		reinterpret_cast<args::program*>(args->modules + max_modules);
	args->num_programs = 0;

	return args;
}

/// Add a module to the kernel args list
inline void
add_module(args::header *args, args::mod_type type, buffer &data)
{
	args::module &m = args->modules[args->num_modules++];
	m.type = type;
	m.location = data.data;
	m.size = data.size;
}

/// The main procedure for the portion of the loader that runs while
/// UEFI is still in control of the machine. (ie, while the loader still
/// has access to boot services.
args::header *
bootloader_main_uefi(
	uefi::handle image,
	uefi::system_table *st,
	console &con)
{
	error::uefi_handler handler(con);
	status_line status(L"Performing UEFI pre-boot");

	uefi::boot_services *bs = st->boot_services;
	uefi::runtime_services *rs = st->runtime_services;
	memory::init_pointer_fixup(bs, rs);

	args::header *args =
		allocate_args_structure(bs, max_modules, max_programs);

	args->magic = args::magic;
	args->version = args::version;
	args->runtime_services = rs;
	args->acpi_table = hw::find_acpi_table(st);
	paging::allocate_tables(args, bs);

	memory::mark_pointer_fixup(&args->runtime_services);

	fs::file disk = fs::get_boot_volume(image, bs);

	const uefi::memory_type mod_type = memory::module_type;
	buffer symbols = loader::load_file(disk, L"symbol table", L"symbol_table.dat",
				memory::module_type);
	add_module(args, args::mod_type::symbol_table, symbols);

	for (auto &desc : program_list) {
		buffer buf = loader::load_file(disk, desc.name, desc.path);
		args::program &program = args->programs[args->num_programs++];
		loader::load_program(program, desc.name, buf, bs);
	}

	for (unsigned i = 0; i < args->num_modules; ++i) {
		args::module &mod = args->modules[i];
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

	args::header *args =
		bootloader_main_uefi(image_handle, st, con);

	args::program &kernel = args->programs[0];
	paging::map_pages(args, kernel.phys_addr, kernel.virt_addr, kernel.size);
	kernel::entrypoint kentry =
		reinterpret_cast<kernel::entrypoint>(kernel.entrypoint);

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

