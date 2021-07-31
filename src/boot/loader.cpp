#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "allocator.h"
#include "console.h"
#include "elf/file.h"
#include "elf/headers.h"
#include "error.h"
#include "fs.h"
#include "init_args.h"
#include "loader.h"
#include "memory.h"
#include "paging.h"
#include "pointer_manipulation.h"
#include "status.h"

namespace init = kernel::init;

namespace boot {
namespace loader {

using memory::alloc_type;

buffer
load_file(
	fs::file &disk,
	const program_desc &desc)
{
	status_line status(L"Loading file", desc.path);

	fs::file file = disk.open(desc.path);
	buffer b = file.load();

	//console::print(L"    Loaded at: 0x%lx, %d bytes\r\n", b.data, b.size);
	return b;
}


static void
create_module(buffer data, const program_desc &desc, bool loaded)
{
	size_t path_len = wstrlen(desc.path);
	init::module_program *mod = g_alloc.allocate_module<init::module_program>(path_len);
	mod->mod_type = init::module_type::program;
	mod->base_address = reinterpret_cast<uintptr_t>(data.pointer);
	if (loaded)
		mod->mod_flags = static_cast<init::module_flags>(
			static_cast<uint8_t>(mod->mod_flags) |
			static_cast<uint8_t>(init::module_flags::no_load));

	// TODO: support non-ascii path characters and do real utf-16 to utf-8
	// conversion
	for (int i = 0; i < path_len; ++i)
		mod->filename[i] = desc.path[i];
	mod->filename[path_len] = 0;
}

init::program *
load_program(
	fs::file &disk,
	const program_desc &desc,
	bool add_module)
{
	status_line status(L"Loading program", desc.name);

	buffer data = load_file(disk, desc);

	if (add_module)
		create_module(data, desc, true);

	elf::file program(data.pointer, data.count);
	if (!program.valid())
		error::raise(uefi::status::load_error, L"ELF file not valid");

	size_t num_sections = 0;
	for (auto &seg : program.programs()) {
		if (seg.type == elf::segment_type::load)
			++num_sections;
	}

	init::program_section *sections = new init::program_section [num_sections];

	size_t next_section = 0;
	for (auto &seg : program.programs()) {
		if (seg.type != elf::segment_type::load)
			continue;

		init::program_section &section = sections[next_section++];

		size_t page_count = memory::bytes_to_pages(seg.mem_size);

		if (seg.mem_size > seg.file_size) {
			void *pages = g_alloc.allocate_pages(page_count, alloc_type::program, true);
			void *source = offset_ptr<void>(data.pointer, seg.offset);
			g_alloc.copy(pages, source, seg.file_size);
			section.phys_addr = reinterpret_cast<uintptr_t>(pages);
		} else {
			section.phys_addr = program.base() + seg.offset;
		}

		section.virt_addr = seg.vaddr;
		section.size = seg.mem_size;
		section.type = static_cast<init::section_flags>(seg.flags);
	}

	init::program *prog = new init::program;
	prog->sections = { .pointer = sections, .count = num_sections };
	prog->phys_base = program.base();
	prog->entrypoint = program.entrypoint();
	return prog;
}

void
load_module(
	fs::file &disk,
	const program_desc &desc)
{
	status_line status(L"Loading module", desc.name);

	buffer data = load_file(disk, desc);
	create_module(data, desc, false);
}

void
verify_kernel_header(init::program &program)
{
	status_line status(L"Verifying kernel header");

    const init::header *header =
        reinterpret_cast<const init::header *>(program.sections[0].phys_addr);

    if (header->magic != init::header_magic)
        error::raise(uefi::status::load_error, L"Bad kernel magic number");

    if (header->length < sizeof(init::header))
        error::raise(uefi::status::load_error, L"Bad kernel header length");

    if (header->version < init::min_header_version)
        error::raise(uefi::status::unsupported, L"Kernel header version not supported");

	console::print(L"    Loaded kernel vserion: %d.%d.%d %x\r\n",
            header->version_major, header->version_minor, header->version_patch,
            header->version_gitsha);

	/*
	for (auto &section : program.sections)
		console::print(L"    Section: p:0x%lx v:0x%lx fs:0x%x ms:0x%x\r\n",
			section.phys_addr, section.virt_addr, section.file_size, section.mem_size);
	*/
}

} // namespace loader
} // namespace boot
