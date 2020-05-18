#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "loader.h"
#include "console.h"
#include "elf.h"
#include "error.h"
#include "memory.h"
#include "paging.h"

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

static void
map_pages(
	paging::page_table *pml4,
	kernel::args::header *args,
	uintptr_t phys, uintptr_t virt,
	size_t bytes)
{
}

kernel::entrypoint
load(
	const void *data, size_t size,
	kernel::args::header *args,
	uefi::boot_services *bs)
{
	status_line status(L"Loading kernel ELF binary");
	const elf::header *header = reinterpret_cast<const elf::header*>(data);

	if (size < sizeof(elf::header) || !is_elfheader_valid(header))
		error::raise(uefi::status::load_error, L"Kernel ELF not valid");

	paging::page_table *pml4 = reinterpret_cast<paging::page_table*>(args->pml4);

	for (int i = 0; i < header->ph_num; ++i) {
		ptrdiff_t offset = header->ph_offset + i * header->ph_entsize;
		const elf::program_header *pheader =
			offset_ptr<elf::program_header>(data, offset);

		if (pheader->type != elf::PT_LOAD)
			continue;

		size_t num_pages = memory::bytes_to_pages(pheader->mem_size);
		void *pages = nullptr;

		try_or_raise(
			bs->allocate_pages(uefi::allocate_type::any_pages,
				memory::kernel_type, num_pages, &pages),
			L"Failed allocating space for kernel code");

		void *data_start = offset_ptr<void>(data, pheader->offset);
		bs->copy_mem(pages, data_start, pheader->mem_size);

		console::print(L"          Kernel section %d physical addr: 0x%lx\r\n", i, pages);
		console::print(L"          Kernel section %d virtual addr:  0x%lx\r\n", i, pheader->vaddr);

		// TODO: map these pages into kernel args' page tables
		// remember to set appropriate RWX permissions
		map_pages(pml4, args, reinterpret_cast<uintptr_t>(pages), pheader->vaddr, pheader->mem_size);
	}

	return reinterpret_cast<kernel::entrypoint>(header->entrypoint);
}

} // namespace loader
} // namespace boot
