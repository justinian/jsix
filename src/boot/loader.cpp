#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "allocator.h"
#include "console.h"
#include "elf.h"
#include "error.h"
#include "fs.h"
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
	const wchar_t *name,
	const wchar_t *path)
{
	status_line status(L"Loading file", name);

	fs::file file = disk.open(path);
	buffer b = file.load();

	//console::print(L"    Loaded at: 0x%lx, %d bytes\r\n", b.data, b.size);
	return b;
}


static bool
is_elfheader_valid(const elf::header *header)
{
	return
		header->magic[0] == 0x7f &&
		header->magic[1] == 'E' &&
		header->magic[2] == 'L' &&
		header->magic[3] == 'F' &&
		header->word_size == elf::word_size &&
		header->endianness == elf::endianness &&
		header->os_abi == elf::os_abi &&
		header->machine == elf::machine &&
		header->header_version == elf::version;
}

void
load_program(
	init::program &program,
	const wchar_t *name,
	buffer data)
{
	status_line status(L"Loading program:", name);

	const elf::header *header = reinterpret_cast<const elf::header*>(data.pointer);
	uintptr_t program_base = reinterpret_cast<uintptr_t>(data.pointer);

	if (data.count < sizeof(elf::header) || !is_elfheader_valid(header))
		error::raise(uefi::status::load_error, L"ELF file not valid");

	size_t num_sections = 0;
	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data.pointer, offset);

		if (pheader->type == elf::PT_LOAD)
			++num_sections;
	}

	init::program_section *sections = new init::program_section [num_sections];

	size_t next_section = 0;
	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data.pointer, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		init::program_section &section = sections[next_section++];

		size_t page_count = memory::bytes_to_pages(pheader->mem_size);

		if (pheader->mem_size > pheader->file_size) {
			void *pages = g_alloc.allocate_pages(page_count, alloc_type::program, true);
			void *source = offset_ptr<void>(data.pointer, pheader->offset);
			g_alloc.copy(pages, source, pheader->file_size);
			section.phys_addr = reinterpret_cast<uintptr_t>(pages);
		} else {
			section.phys_addr = program_base + pheader->offset;
		}

		section.virt_addr = pheader->vaddr;
		section.size = pheader->mem_size;
		section.type = static_cast<init::section_flags>(pheader->flags);
	}

	program.sections = { .pointer = sections, .count = num_sections };
	program.phys_base = program_base;
	program.entrypoint = header->entrypoint;
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
