#include "loader.h"
#include "utility.h"

static CHAR16 kernel_name[] = KERNEL_FILENAME;

EFI_STATUS
loader_load_kernel(
	EFI_BOOT_SERVICES *bootsvc,
	void **kernel_image,
	uint64_t *kernel_length,
	void **kernel_data,
	uint64_t *data_length)
{
	if (kernel_image == 0 || kernel_length == 0)
		CHECK_EFI_STATUS_OR_RETURN(EFI_INVALID_PARAMETER, "NULL kernel_image or length pointer");

	if (kernel_data == 0 || data_length == 0)
		CHECK_EFI_STATUS_OR_RETURN(EFI_INVALID_PARAMETER, "NULL kernel_data or length pointer");

	EFI_STATUS status;
	EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_HANDLE *handles = NULL;
	UINTN handleCount = 0;

	status = bootsvc->LocateHandleBuffer(ByProtocol, &guid, NULL, &handleCount, &handles);
	CHECK_EFI_STATUS_OR_RETURN(status, "LocateHandleBuffer");

	for (unsigned i = 0; i < handleCount; ++i) {
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem = NULL;

		status = bootsvc->HandleProtocol(handles[i], &guid, (void **)&fileSystem);
		CHECK_EFI_STATUS_OR_RETURN(status, "HandleProtocol");

		EFI_FILE_PROTOCOL *root = NULL;
		status = fileSystem->OpenVolume(fileSystem, &root);
		CHECK_EFI_STATUS_OR_RETURN(status, "OpenVolume");

		EFI_FILE_PROTOCOL *file = NULL;
		status = root->Open(root, &file, kernel_name, EFI_FILE_MODE_READ,
							EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

		if (!EFI_ERROR(status)) {
			void *buffer = NULL;
			EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
			UINTN buffer_size = sizeof(EFI_FILE_INFO) + sizeof(kernel_name);

			status = bootsvc->AllocatePool(EfiLoaderCode, buffer_size, &buffer);
			CHECK_EFI_STATUS_OR_RETURN(status, "Allocating kernel file info memory");

			status = file->GetInfo(file, &file_info_guid, &buffer_size, buffer);
			CHECK_EFI_STATUS_OR_RETURN(status, "Getting kernel file info");

			buffer_size = ((EFI_FILE_INFO *)buffer)->FileSize;

			status = bootsvc->FreePool(buffer);
			CHECK_EFI_STATUS_OR_RETURN(status, "Freeing kernel file info memory");

			UINTN page_count = ((buffer_size - 1) / 0x1000) + 1;
			EFI_PHYSICAL_ADDRESS addr = KERNEL_PHYS_ADDRESS;
			EFI_MEMORY_TYPE mem_type = KERNEL_MEMTYPE; // Special value to tell the kernel it's here
			status = bootsvc->AllocatePages(AllocateAddress, mem_type, page_count, &addr);
			if (status == EFI_NOT_FOUND) {
				// couldn't get the address we wanted, try loading the kernel anywhere
				status =
					bootsvc->AllocatePages(AllocateAnyPages, mem_type, page_count, &addr);
			}
			CHECK_EFI_STATUS_OR_RETURN(status, "Allocating kernel pages");

			buffer = (void *)addr;
			status = file->Read(file, &buffer_size, buffer);
			CHECK_EFI_STATUS_OR_RETURN(status, "Reading kernel file");

			status = file->Close(file);
			CHECK_EFI_STATUS_OR_RETURN(status, "Closing kernel file handle");

			*kernel_length = buffer_size;
			*kernel_image = buffer;

			addr += page_count * 0x1000; // Get the next page after the kernel pages
			mem_type = KERNEL_DATA_MEMTYPE; // Special value for kernel data
			page_count = ((*data_length - 1) / 0x1000) + 1;
			status = bootsvc->AllocatePages(AllocateAddress, mem_type, page_count, &addr);
			if (status == EFI_NOT_FOUND) {
				// couldn't get the address we wanted, try loading anywhere
				status =
					bootsvc->AllocatePages(AllocateAnyPages, mem_type, page_count, &addr);
			}
			CHECK_EFI_STATUS_OR_RETURN(status, "Allocating kernel data pages");

			*data_length = page_count * 0x1000;
			*kernel_data = (void *)addr;
			bootsvc->SetMem(*kernel_data, *data_length, 0);

			return EFI_SUCCESS;
		}
	}

	return EFI_NOT_FOUND;
}
