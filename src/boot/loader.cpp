#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "loader.h"
#include "console.h"
#include "elf.h"
#include "error.h"
#include "fs.h"
#include "memory.h"
#include "paging.h"
#include "status.h"

namespace init = kernel::init;

namespace boot {
namespace loader {

buffer
load_file(
	fs::file &disk,
	const wchar_t *name,
	const wchar_t *path,
	uefi::memory_type type)
{
	status_line status(L"Loading file", name);

	fs::file file = disk.open(path);
	buffer b = file.load(type);

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
	buffer data,
	uefi::boot_services *bs)
{
	status_line status(L"Loading program:", name);
	const elf::header *header = reinterpret_cast<const elf::header*>(data.data);

	if (data.size < sizeof(elf::header) || !is_elfheader_valid(header))
		error::raise(uefi::status::load_error, L"ELF file not valid");

	uintptr_t prog_base = uintptr_t(-1);
	uintptr_t prog_end = 0;

	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data.data, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		uintptr_t end = pheader->vaddr + pheader->mem_size;
		if (pheader->vaddr < prog_base) prog_base = pheader->vaddr;
		if (end > prog_end) prog_end = end;
	}

	size_t total_size = prog_end - prog_base;
	size_t num_pages = memory::bytes_to_pages(total_size);
	void *pages = nullptr;

	try_or_raise(
		bs->allocate_pages(uefi::allocate_type::any_pages,
			uefi::memory_type::loader_data, num_pages, &pages),
		L"Failed allocating space for program");

	bs->set_mem(pages, total_size, 0);

	program.base = prog_base;
	program.total_size = total_size;
	program.num_sections = 0;
	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data.data, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		init::program_section &section = program.sections[program.num_sections++];

		void *src_start = offset_ptr<void>(data.data, pheader->offset);
		void *dest_start = offset_ptr<void>(pages, pheader->vaddr - prog_base);

		bs->copy_mem(dest_start, src_start, pheader->file_size);
		section.phys_addr = reinterpret_cast<uintptr_t>(dest_start);
		section.virt_addr = pheader->vaddr;
		section.size = pheader->mem_size;
		section.type = static_cast<init::section_flags>(pheader->flags);
	}

	program.entrypoint = header->entrypoint;
}

void
verify_kernel_header(
	init::program &program,
	uefi::boot_services *bs)
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

	console::print(L"    Loaded kernel vserion: %d.%d.%d %lx\r\n",
            header->version_major, header->version_minor, header->version_patch,
            header->version_gitsha);
}

} // namespace loader
} // namespace boot
