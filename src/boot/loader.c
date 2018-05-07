#include "elf.h"
#include "guids.h"
#include "loader.h"
#include "memory.h"
#include "utility.h"

#define PAGE_SIZE 0x1000

static CHAR16 kernel_name[] = KERNEL_FILENAME;
static CHAR16 font_name[] = KERNEL_FONT;

EFI_STATUS
loader_alloc_pages(
	EFI_BOOT_SERVICES *bootsvc,
	EFI_MEMORY_TYPE mem_type,
	size_t *length,
	void **pages)
{
	EFI_STATUS status;

	size_t page_count = ((*length - 1) / PAGE_SIZE) + 1;
	EFI_PHYSICAL_ADDRESS addr = (EFI_PHYSICAL_ADDRESS)*pages;

	status = bootsvc->AllocatePages(AllocateAddress, mem_type, page_count, &addr);
	if (status == EFI_NOT_FOUND || status == EFI_OUT_OF_RESOURCES) {
		// couldn't get the address we wanted, try loading the kernel anywhere
		status =
			bootsvc->AllocatePages(AllocateAnyPages, mem_type, page_count, &addr);
	}
	CHECK_EFI_STATUS_OR_RETURN(status,
		L"Allocating %d kernel pages type %x",
		page_count, mem_type);

	*length = page_count * PAGE_SIZE;
	*pages = (void *)addr;

	return EFI_SUCCESS;
}

EFI_STATUS
loader_load_font(
	EFI_BOOT_SERVICES *bootsvc,
	EFI_FILE_PROTOCOL *root,
	struct loader_data *data)
{
	EFI_STATUS status;

	EFI_FILE_PROTOCOL *file = NULL;
	status = root->Open(root, &file, (CHAR16 *)font_name, EFI_FILE_MODE_READ,
						EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

	if (status == EFI_NOT_FOUND)
		return status;

	CHECK_EFI_STATUS_OR_RETURN(status, L"Opening file %s", font_name);

	char info[sizeof(EFI_FILE_INFO) + 100];
	size_t info_length = sizeof(info);

	status = file->GetInfo(file, &guid_file_info, &info_length, info);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Getting file info");

	data->font_length = ((EFI_FILE_INFO *)info)->FileSize;

	status = loader_alloc_pages(
			bootsvc,
			KERNEL_FONT_MEMTYPE,
			&data->font_length,
			&data->font);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Allocating pages");

	status = file->Read(file, &data->font_length, data->font);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Reading file");

	status = file->Close(file);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Closing file handle");

	return EFI_SUCCESS;
}


EFI_STATUS
loader_load_elf(
	EFI_BOOT_SERVICES *bootsvc,
	EFI_FILE_PROTOCOL *root,
	struct loader_data *data)
{
	EFI_STATUS status;

	EFI_FILE_PROTOCOL *file = NULL;
	status = root->Open(root, &file, (CHAR16 *)kernel_name, EFI_FILE_MODE_READ,
						EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

	if (status == EFI_NOT_FOUND)
		return status;

	uint64_t length = 0;
	data->kernel = 0;
	data->kernel_entry = 0;
	data->kernel_length = 0;

	CHECK_EFI_STATUS_OR_RETURN(status, L"Opening file %s", kernel_name);

	struct elf_header header;

	length = sizeof(struct elf_header);
	status = file->Read(file, &length, &header);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Reading ELF header");

	if (length < sizeof(struct elf_header))
		CHECK_EFI_STATUS_OR_RETURN(EFI_LOAD_ERROR, L"Incomplete read of ELF header");

	static const char expected[] = {0x7f, 'E', 'L', 'F'};
	for (int i = 0; i < sizeof(expected); ++i) {
		if (header.ident.magic[i] != expected[i])
			CHECK_EFI_STATUS_OR_RETURN(EFI_LOAD_ERROR, L"Bad ELF magic number");
	}

	if (header.ident.word_size != ELF_WORDSIZE)
		CHECK_EFI_STATUS_OR_RETURN(EFI_LOAD_ERROR, L"ELF load error: 32 bit ELF not supported");

	if (header.ph_entsize != sizeof(struct elf_program_header))
		CHECK_EFI_STATUS_OR_RETURN(EFI_LOAD_ERROR, L"ELF load error: program header size mismatch");

	if (header.ident.version != ELF_VERSION ||
		header.version != ELF_VERSION)
		CHECK_EFI_STATUS_OR_RETURN(EFI_LOAD_ERROR, L"ELF load error: wrong ELF version");

	if (header.ident.endianness != 1 ||
		header.ident.os_abi != 0 ||
		header.machine != 0x3e)
		CHECK_EFI_STATUS_OR_RETURN(EFI_LOAD_ERROR, L"ELF load error: wrong machine architecture");

	data->kernel_entry = (void *)header.entrypoint;

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
		status = loader_alloc_pages(bootsvc, KERNEL_MEMTYPE, &length, &addr);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Allocating kernel pages");

		if (data->kernel == 0)
			data->kernel = addr;
		data->kernel_length = (uint64_t)addr + length - (uint64_t)data->kernel;
	}

	struct elf_section_header sec_header;
	for (int i = 0; i < header.sh_num; ++i) {
		status = file->SetPosition(file, header.sh_offset + i * header.sh_entsize);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Setting ELF file position");

		length = header.sh_entsize;
		status = file->Read(file, &length, &sec_header);
		CHECK_EFI_STATUS_OR_RETURN(status, L"Reading ELF section header");

		if ((sec_header.flags & ELF_SHF_ALLOC) == 0) continue;

		void *addr = (void *)(sec_header.addr - KERNEL_VIRT_ADDRESS);

		if (sec_header.type == ELF_ST_PROGBITS) {
			status = file->SetPosition(file, sec_header.offset);
			CHECK_EFI_STATUS_OR_RETURN(status, L"Setting ELF file position");

			length = sec_header.size;
			status = file->Read(file, &length, addr);
			CHECK_EFI_STATUS_OR_RETURN(status, L"Reading file");
			con_printf(L"Loaded %lx bytes %lx - %lx\n",
					sec_header.size,
					addr,
					sec_header.addr + sec_header.size - KERNEL_VIRT_ADDRESS);
		} else if (sec_header.type == ELF_ST_NOBITS) {
			bootsvc->SetMem(addr, sec_header.size, 0);
			con_printf(L"Zeroed %lx bytes %lx - %lx\n",
					sec_header.size,
					addr,
					sec_header.addr + sec_header.size - KERNEL_VIRT_ADDRESS);
		}
	}

	status = file->Close(file);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Closing file handle");

	return EFI_SUCCESS;
}


EFI_STATUS
loader_load_kernel(
	EFI_BOOT_SERVICES *bootsvc,
	struct loader_data *data)
{
	if (data == NULL)
		CHECK_EFI_STATUS_OR_RETURN(EFI_INVALID_PARAMETER, L"NULL loader_data");

	EFI_STATUS status;
	EFI_HANDLE *handles = NULL;
	size_t handleCount = 0;

	status = bootsvc->LocateHandleBuffer(ByProtocol, &guid_simple_filesystem, NULL, &handleCount, &handles);
	CHECK_EFI_STATUS_OR_RETURN(status, L"LocateHandleBuffer");

	for (unsigned i = 0; i < handleCount; ++i) {
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem = NULL;

		status = bootsvc->HandleProtocol(handles[i], &guid_simple_filesystem, (void **)&fileSystem);
		CHECK_EFI_STATUS_OR_RETURN(status, L"HandleProtocol");

		EFI_FILE_PROTOCOL *root = NULL;
		status = fileSystem->OpenVolume(fileSystem, &root);
		CHECK_EFI_STATUS_OR_RETURN(status, L"OpenVolume");

		status = loader_load_elf(bootsvc, root, data);
		if (status == EFI_NOT_FOUND)
			continue;

		CHECK_EFI_STATUS_OR_RETURN(status, L"loader_load_file: %s", kernel_name);

		data->font = (void *)((uint64_t)data->kernel + data->kernel_length);
		status = loader_load_font(bootsvc, root, data);

		CHECK_EFI_STATUS_OR_RETURN(status, L"loader_load_file: %s", font_name);

		data->data = (void *)((uint64_t)data->font + data->font_length);
		data->data_length += PAGE_SIZE; // extra page for map growth
		status = loader_alloc_pages(
				bootsvc,
				KERNEL_DATA_MEMTYPE,
				&data->data_length,
				&data->data);
		CHECK_EFI_STATUS_OR_RETURN(status, L"loader_alloc_pages: kernel data");

		return EFI_SUCCESS;
	}

	return EFI_NOT_FOUND;
}
