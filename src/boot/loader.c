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
	void **pages,
	void **next)
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
	CHECK_EFI_STATUS_OR_RETURN(status, L"Allocating kernel pages type %x", mem_type);

	*length = page_count * PAGE_SIZE;
	*pages = (void *)addr;
	*next = (void*)(addr + *length);

	return EFI_SUCCESS;
}

EFI_STATUS
loader_load_file(
	EFI_BOOT_SERVICES *bootsvc,
	EFI_FILE_PROTOCOL *root,
	const CHAR16 *filename,
	EFI_MEMORY_TYPE mem_type,
	void **data,
	size_t *length,
	void **next)
{
	EFI_STATUS status;

	EFI_FILE_PROTOCOL *file = NULL;
	status = root->Open(root, &file, (CHAR16 *)filename, EFI_FILE_MODE_READ,
						EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

	if (status == EFI_NOT_FOUND)
		return status;

	CHECK_EFI_STATUS_OR_RETURN(status, L"Opening file %s", filename);

	char info[sizeof(EFI_FILE_INFO) + 100];
	size_t info_length = sizeof(info);

	status = file->GetInfo(file, &guid_file_info, &info_length, info);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Getting file info");

	*length = ((EFI_FILE_INFO *)info)->FileSize;

	status = loader_alloc_pages(bootsvc, mem_type, length, data, next);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Allocating pages");

	status = file->Read(file, length, *data);
	CHECK_EFI_STATUS_OR_RETURN(status, L"Reading file");

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

		void *next = NULL;

		data->kernel_image = (void *)KERNEL_PHYS_ADDRESS;
		status = loader_load_file(
				bootsvc,
				root,
				kernel_name,
				KERNEL_MEMTYPE,
				&data->kernel_image,
				&data->kernel_image_length,
				&next);
		if (status == EFI_NOT_FOUND)
			continue;

		CHECK_EFI_STATUS_OR_RETURN(status, L"loader_load_file: %s", kernel_name);

		data->screen_font = next;
		status = loader_load_file(
				bootsvc,
				root,
				font_name,
				KERNEL_FONT_MEMTYPE,
				&data->screen_font,
				&data->screen_font_length,
				&next);

		CHECK_EFI_STATUS_OR_RETURN(status, L"loader_load_file: %s", font_name);

		data->kernel_data = next;
		data->kernel_data_length += PAGE_SIZE; // extra page for map growth
		status = loader_alloc_pages(
				bootsvc,
				KERNEL_DATA_MEMTYPE,
				&data->kernel_data_length,
				&data->kernel_data,
				&next);
		CHECK_EFI_STATUS_OR_RETURN(status, L"loader_alloc_pages: kernel data");

		return EFI_SUCCESS;
	}

	return EFI_NOT_FOUND;
}
