#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "loader.h"
#include "console.h"
#include "elf.h"
#include "error.h"
#include "memory.h"

namespace boot {
namespace loader {

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

loaded_elf
load(
	const void *data,
	size_t size,
	uefi::boot_services *bs)
{
	status_line status(L"Loading kernel ELF binary");
	const elf::header *header = reinterpret_cast<const elf::header*>(data);

	if (size < sizeof(elf::header) || !is_elfheader_valid(header))
		error::raise(uefi::status::load_error, L"Kernel ELF not valid");

	uintptr_t kernel_start = 0;
	uintptr_t kernel_end = 0;
	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		if (kernel_start == 0 || pheader->vaddr < kernel_start)
			kernel_start = pheader->vaddr;

		if (pheader->vaddr + pheader->mem_size > kernel_end)
			kernel_end = pheader->vaddr + pheader->mem_size;
	}

	void *pages = nullptr;
	size_t num_pages = memory::bytes_to_pages(kernel_end - kernel_start);
	try_or_raise(
		bs->allocate_pages(uefi::allocate_type::any_pages,
			memory::kernel_type, num_pages, &pages),
		L"Failed allocating space for kernel code");

	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		void *data_start = offset_ptr<void>(data, pheader->offset);
		void *program_start = offset_ptr<void>(pages, pheader->vaddr - kernel_start);
		bs->copy_mem(program_start, data_start, pheader->mem_size);
	}

	loaded_elf result;
	result.data = pages;
	result.vaddr = kernel_start;
	result.entrypoint = header->entrypoint;
	console::print(L"          Kernel loaded at: 0x%lx\r\n", result.data);
	console::print(L"    Kernel virtual address: 0x%lx\r\n", result.vaddr);
	console::print(L"         Kernel entrypoint: 0x%lx\r\n", result.entrypoint);

	return result;
}

} // namespace loader
} // namespace boot
