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

	if (data.count < sizeof(elf::header) || !is_elfheader_valid(header))
		error::raise(uefi::status::load_error, L"ELF file not valid");

	size_t num_sections = 0;
	uintptr_t prog_base = uintptr_t(-1);
	uintptr_t prog_end = 0;

	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data.pointer, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		++num_sections;
		uintptr_t end = pheader->vaddr + pheader->mem_size;
		if (pheader->vaddr < prog_base) prog_base = pheader->vaddr;
		if (end > prog_end) prog_end = end;
	}

	init::program_section *sections = new init::program_section [num_sections];
	program.sections = { .pointer = sections, .count = num_sections };

	size_t total_size = prog_end - prog_base;
	size_t num_pages = memory::bytes_to_pages(total_size);
	void *pages = g_alloc.allocate_pages(num_pages, alloc_type::program, true);
	program.phys_base = reinterpret_cast<uintptr_t>(pages);

	size_t next_section = 0;
	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data.pointer, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		init::program_section &section = program.sections[next_section++];

		void *src_start = offset_ptr<void>(data.pointer, pheader->offset);
		void *dest_start = offset_ptr<void>(pages, pheader->vaddr - prog_base);

		g_alloc.copy(dest_start, src_start, pheader->file_size);
		section.phys_addr = reinterpret_cast<uintptr_t>(dest_start);
		section.virt_addr = pheader->vaddr;
		section.size = pheader->mem_size;
		section.type = static_cast<init::section_flags>(pheader->flags);
	}

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
}

} // namespace loader
} // namespace boot
