#include <uefi/types.h>
#include <uefi/guid.h>
#include <uefi/tables.h>
#include <uefi/protos/simple_text_output.h>

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "allocator.h"
#include "console.h"
#include "error.h"
#include "fs.h"
#include "hardware.h"
#include "loader.h"
#include "memory.h"
#include "memory_map.h"
#include "paging.h"
#include "status.h"
#include "video.h"

#include "kernel_args.h"

namespace kernel {
#include "kernel_memory.h"
}

namespace init = kernel::init;

namespace boot {

const loader::program_desc kern_desc = {L"kernel", L"jsix.elf"};
const loader::program_desc init_desc = {L"init server", L"srv.init.elf"};
const loader::program_desc fb_driver = {L"UEFI framebuffer driver", L"drv.uefi_fb.elf"};

const loader::program_desc extra_programs[] = {
	{L"test application", L"testapp.elf"},
};

/// Change a pointer to point to the higher-half linear-offset version
/// of the address it points to.
template <typename T>
void change_pointer(T *&pointer)
{
	pointer = offset_ptr<T>(pointer, kernel::memory::page_offset);
}

/// The main procedure for the portion of the loader that runs while
/// UEFI is still in control of the machine. (ie, while the loader still
/// has access to boot services.)
init::args *
uefi_preboot(uefi::handle image, uefi::system_table *st)
{
	uefi::boot_services *bs = st->boot_services;
	uefi::runtime_services *rs = st->runtime_services;

	status_line status {L"Performing UEFI pre-boot"};

	hw::check_cpu_supported();
	memory::init_pointer_fixup(bs, rs);

	init::args *args = new init::args;
	g_alloc.zero(args, sizeof(init::args));

	args->magic = init::args_magic;
	args->version = init::args_version;
	args->runtime_services = rs;
	args->acpi_table = hw::find_acpi_table(st);
	memory::mark_pointer_fixup(&args->runtime_services);

	paging::allocate_tables(args);

	return args;
}

/// Load the kernel and other programs from disk
void
load_resources(init::args *args, video::screen *screen, uefi::handle image, uefi::boot_services *bs)
{
	status_line status {L"Loading programs"};

	fs::file disk = fs::get_boot_volume(image, bs);

	if (screen) {
		video::make_module(screen);
		loader::load_module(disk, fb_driver);
	}

	args->symbol_table = loader::load_file(disk, {L"symbol table", L"symbol_table.dat"});

	args->kernel = loader::load_program(disk, kern_desc, true);
	args->init = loader::load_program(disk, init_desc);

	for (auto &desc : extra_programs)
		loader::load_module(disk, desc);

    loader::verify_kernel_header(*args->kernel);

}

memory::efi_mem_map
uefi_exit(init::args *args, uefi::handle image, uefi::boot_services *bs)
{
	status_line status {L"Exiting UEFI", nullptr, false};

	memory::efi_mem_map map;
	map.update(*bs);

	args->mem_map = memory::build_kernel_map(map);
	args->frame_blocks = memory::build_frame_blocks(args->mem_map);

	map.update(*bs);
	try_or_raise(
		bs->exit_boot_services(image, map.key),
		L"Failed to exit boot services");

	return map;
}

} // namespace boot

/// The UEFI entrypoint for the loader.
extern "C" uefi::status
efi_main(uefi::handle image, uefi::system_table *st)
{
	using namespace boot;

	uefi::boot_services *bs = st->boot_services;
	console con(st->con_out);

	init::allocation_register *allocs = nullptr;
	init::modules_page *modules = nullptr;
	memory::allocator::init(allocs, modules, bs);

	video::screen *screen = video::pick_mode(bs);
	con.announce();

	init::args *args = uefi_preboot(image, st);
	load_resources(args, screen, image, bs);
	memory::efi_mem_map map = uefi_exit(args, image, st->boot_services);

	args->allocations = allocs;
	args->modules = reinterpret_cast<uintptr_t>(modules);

	status_bar status {screen}; // Switch to fb status display

	// Map the kernel to the appropriate address
	init::program &kernel = *args->kernel;
	for (auto &section : kernel.sections)
		paging::map_section(args, section);

	memory::fix_frame_blocks(args);

	init::entrypoint kentry =
		reinterpret_cast<init::entrypoint>(kernel.entrypoint);
	//status.next();

	hw::setup_control_regs();
	memory::virtualize(args->pml4, map, st->runtime_services);
	//status.next();

	change_pointer(args);
	change_pointer(args->pml4);
	change_pointer(args->symbol_table.pointer);

	change_pointer(args->kernel);
	change_pointer(args->kernel->sections.pointer);
	change_pointer(args->init);
	change_pointer(args->init->sections.pointer);

	//status.next();

	kentry(args);
	debug_break();
	return uefi::status::unsupported;
}

