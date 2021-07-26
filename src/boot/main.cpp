#include <uefi/types.h>
#include <uefi/guid.h>
#include <uefi/tables.h>
#include <uefi/protos/simple_text_output.h>

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "allocator.h"
#include "console.h"
#include "cpu/cpu_id.h"
#include "error.h"
#include "fs.h"
#include "hardware.h"
#include "loader.h"
#include "memory.h"
#include "memory_map.h"
#include "paging.h"
#include "status.h"

#include "kernel_args.h"

namespace kernel {
#include "kernel_memory.h"
}

namespace init = kernel::init;

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
	{L"test application", L"testapp.elf"},
	{L"UEFI framebuffer driver", L"drv.uefi_fb.elf"},
};

/// Change a pointer to point to the higher-half linear-offset version
/// of the address it points to.
template <typename T>
void change_pointer(T *&pointer)
{
	pointer = offset_ptr<T>(pointer, kernel::memory::page_offset);
}

/// Add a module to the kernel args list
inline void
add_module(init::args *args, init::mod_type type, buffer &data)
{
	init::module &m = args->modules[args->modules.count++];
	m.type = type;
	m.location = data.pointer;
	m.size = data.count;

	change_pointer(m.location);
}

/// Check that all required cpu features are supported
void
check_cpu_supported()
{
	status_line status {L"Checking CPU features"};

	cpu::cpu_id cpu;
	uint64_t missing = cpu.missing();
	if (missing) {
#define CPU_FEATURE_OPT(...)
#define CPU_FEATURE_REQ(name, ...) \
		if (!cpu.has_feature(cpu::feature::name)) { \
			status::fail(L"CPU required feature " L ## #name, uefi::status::unsupported); \
		}
#include "cpu/features.inc"
#undef CPU_FEATURE_REQ
#undef CPU_FEATURE_OPT

		error::raise(uefi::status::unsupported, L"CPU not supported");
	}
}

/// The main procedure for the portion of the loader that runs while
/// UEFI is still in control of the machine. (ie, while the loader still
/// has access to boot services.)
init::args *
uefi_preboot(uefi::handle image, uefi::system_table *st)
{
	status_line status {L"Performing UEFI pre-boot"};

	uefi::boot_services *bs = st->boot_services;
	uefi::runtime_services *rs = st->runtime_services;

	memory::init_allocator(bs);
	memory::init_pointer_fixup(bs, rs);

	init::args *args = new init::args;
	g_alloc.zero(args, sizeof(init::args));
	args->programs.pointer = new init::program[5];
	args->modules.pointer = new init::module[5];

	args->magic = init::args_magic;
	args->version = init::args_version;
	args->runtime_services = rs;
	args->acpi_table = hw::find_acpi_table(st);
	memory::mark_pointer_fixup(&args->runtime_services);

	paging::allocate_tables(args);

	fs::file disk = fs::get_boot_volume(image, bs);

	buffer symbols = loader::load_file(disk, L"symbol table", L"symbol_table.dat");
	add_module(args, init::mod_type::symbol_table, symbols);

	for (auto &desc : program_list) {
		buffer buf = loader::load_file(disk, desc.name, desc.path);
		init::program &program = args->programs[args->programs.count++];
		loader::load_program(program, desc.name, buf);
	}

    // First program *must* be the kernel
    loader::verify_kernel_header(args->programs[0]);

	return args;
}

memory::efi_mem_map
uefi_exit(init::args *args, uefi::handle image, uefi::boot_services *bs)
{
	status_line status {L"Exiting UEFI", nullptr, false};

	memory::efi_mem_map map;
	map.update(*bs);

	args->mem_map = memory::build_kernel_map(map);
	args->frame_blocks = memory::build_frame_blocks(args->mem_map);
	args->allocations = g_alloc.get_register();

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
	console con(st->boot_services, st->con_out);
	check_cpu_supported();

	init::args *args = uefi_preboot(image, st);
	memory::efi_mem_map map = uefi_exit(args, image, st->boot_services);

	args->video = con.fb();
	status_bar status {con.fb()}; // Switch to fb status display

	// Map the kernel to the appropriate address
	init::program &kernel = args->programs[0];
	for (auto &section : kernel.sections)
		paging::map_section(args, section);

	memory::fix_frame_blocks(args);

	init::entrypoint kentry =
		reinterpret_cast<init::entrypoint>(kernel.entrypoint);
	status.next();

	hw::setup_control_regs();
	memory::virtualize(args->pml4, map, st->runtime_services);
	status.next();

	change_pointer(args);
	change_pointer(args->pml4);
	change_pointer(args->modules.pointer);
	change_pointer(args->programs.pointer);
	for (auto &program : args->programs)
		change_pointer(program.sections.pointer);

	status.next();

	kentry(args);
	debug_break();
	return uefi::status::unsupported;
}

