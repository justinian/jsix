#include "loader.h"
#include "utility.h"

static CHAR16 kernel_name[] = KERNEL_FILENAME;

static EFI_STATUS loader_load_file(EFI_FILE_PROTOCOL *root, void **kernel_image, UINT64 *len) {
	EFI_STATUS status;

	EFI_FILE_PROTOCOL *handle = 0;
	status = root->Open(root, &handle, kernel_name, EFI_FILE_MODE_READ, 0);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to open kernel file handle");

	EFI_FILE_INFO *info = LibFileInfo(handle);
    if (info->FileSize == 0)
        return EFI_NOT_FOUND;

    UINTN count = ((info->FileSize - 1) / 0x1000) + 1;
    EFI_PHYSICAL_ADDRESS addr = 0x100000; // Try to load the kernel in at 1MiB
    EFI_MEMORY_TYPE memType = 0xFFFFFFFF;  // Special value to tell the kernel it's here
    status = ST->BootServices->AllocatePages(AllocateAddress, memType, count, &addr);
    if (status == EFI_NOT_FOUND) {
        // couldn't get the address we wanted, try loading the kernel anywhere
        status = ST->BootServices->AllocatePages(AllocateAnyPages, memType, count, &addr);
    }
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to allocate kernel pages");

    UINTN buffer_size = count * 0x1000;
    void *buffer = (void*)addr;
    status = handle->Read(handle, &buffer_size, &buffer);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to read kernel to memory");

	*len = buffer_size;
	*kernel_image = buffer;
	return EFI_SUCCESS;
}

EFI_STATUS loader_load_kernel(void **kernel_image, UINT64 *len) {
    if (kernel_image == 0 || len == 0)
        CHECK_EFI_STATUS_OR_RETURN(EFI_INVALID_PARAMETER, "NULL kernel_image pointer or size");

	EFI_STATUS status;

	// First, find all the handles that support the filesystem protocol. Call
	UINTN size = 0;
	EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	status = ST->BootServices->LocateHandle(ByProtocol, &fs_guid, NULL, &size, NULL);
	if (status != EFI_BUFFER_TOO_SMALL) {
		CHECK_EFI_STATUS_OR_RETURN(status, "Failed to find number of filesystem handles");
	} else if (size == 0) {
		CHECK_EFI_STATUS_OR_RETURN(EFI_NO_MEDIA, "Found zero filesystem handles");
	}

	EFI_HANDLE *buffer = 0;
    status = ST->BootServices->AllocatePool(EfiLoaderData, size, (void**)&buffer);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to allocate buffer of filesystem handles");

	status = ST->BootServices->LocateHandle(ByProtocol, &fs_guid, NULL, &size, buffer);
	CHECK_EFI_STATUS_OR_RETURN(status, "Failed to find filesystem handles");

	unsigned num_fs = size / sizeof(EFI_HANDLE);
	EFI_HANDLE *fss = (EFI_HANDLE*)buffer;

	for (unsigned i = 0; i < num_fs; ++i) {
		EFI_FILE_HANDLE root = LibOpenRoot(fss[i]);

		status = loader_load_file(root, kernel_image, len);
        if (status == EFI_NOT_FOUND)
            continue;

		CHECK_EFI_STATUS_OR_RETURN(status, "Failed to load kernel");
        return EFI_SUCCESS;
	}

    return EFI_NOT_FOUND;
}
