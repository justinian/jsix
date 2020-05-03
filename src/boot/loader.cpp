#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "loader.h"
#include "console.h"
#include "elf.h"
#include "error.h"

namespace boot {
namespace loader {

static bool
is_elfheader_valid(const elf::header *header)
{
	return false;
}

kernel::entrypoint
load_elf(
	const void *data,
	size_t size,
	uefi::boot_services *bs)
{
	status_line status(L"Loading kernel ELF binary");

	if (size < sizeof(elf::header) ||
		!is_elfheader_valid(reinterpret_cast<const elf::header*>(data)))
		error::raise(uefi::status::load_error, L"Kernel ELF not valid");

	/*
	struct elf_program_header prog_header;
	for (int i = 0; i < header.ph_num; ++i) {

		status = file->SetPosition(file, header.ph_offset + i * header.ph_entsize);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Setting ELF file position");

		length = header.ph_entsize;
		status = file->Read(file, &length, &prog_header);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Reading ELF program header");

		if (prog_header.type != ELF_PT_LOAD) continue;

		length = prog_header.mem_size;
		void *addr = (void *)(prog_header.vaddr - KERNEL_VIRT_ADDRESS);
		status = loader_alloc_pages(bootsvc, memtype_kernel, &length, &addr);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Allocating kernel pages");

		if (data->kernel == 0)
			data->kernel = addr;
		data->kernel_length = (uint64_t)addr + length - (uint64_t)data->kernel;
	}
	con_debug(L"Read %d ELF program headers", header.ph_num);

	struct elf_section_header sec_header;
	for (int i = 0; i < header.sh_num; ++i) {
		status = file->SetPosition(file, header.sh_offset + i * header.sh_entsize);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Setting ELF file position");

		length = header.sh_entsize;
		status = file->Read(file, &length, &sec_header);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Reading ELF section header");

		if ((sec_header.flags & ELF_SHF_ALLOC) == 0) {
			continue;
		}

		void *addr = (void *)(sec_header.addr - KERNEL_VIRT_ADDRESS);

		if (sec_header.type == ELF_ST_PROGBITS) {
			status = file->SetPosition(file, sec_header.offset);
			CHECK_EFI_STATUS_OR_RETURN(status, L"Setting ELF file position");

			length = sec_header.size;
			status = file->Read(file, &length, addr);
			CHECK_EFI_STATUS_OR_RETURN(status, L"Reading file");
		} else if (sec_header.type == ELF_ST_NOBITS) {
			bootsvc->SetMem(addr, sec_header.size, 0);
		}
	}
	con_debug(L"Read %d ELF section headers", header.ph_num);

	status = file->Close(file);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Closing file handle");

	return reinterpret_cast<kernel::entrypoint>(kernel.entrypoint());
	*/

	return nullptr;
}

} // namespace loader
} // namespace boot
