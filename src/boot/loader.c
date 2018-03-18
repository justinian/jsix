#include "loader.h"
#include "utility.h"

static CHAR16 kernel_name[] = KERNEL_FILENAME;

EFI_STATUS loader_load_kernel(void **kernel_image, uint64_t *length) {
    if (kernel_image == 0 || length == 0)
        CHECK_EFI_STATUS_OR_RETURN(EFI_INVALID_PARAMETER, "NULL kernel_image or length pointer");

    EFI_STATUS status;
    EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_HANDLE *handles = NULL;
    UINTN handleCount = 0;

    status = ST->BootServices->LocateHandleBuffer(
            ByProtocol, &guid, NULL, &handleCount, &handles);
	CHECK_EFI_STATUS_OR_RETURN(status, "LocateHandleBuffer");

    for (unsigned i=0; i<handleCount; ++i) {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fileSystem = NULL;

        status = ST->BootServices->HandleProtocol(
                handles[i], &guid, (void**)&fileSystem);
        CHECK_EFI_STATUS_OR_RETURN(status, "HandleProtocol");

        EFI_FILE_PROTOCOL *root = NULL;
        status = fileSystem->OpenVolume(fileSystem, &root);
        CHECK_EFI_STATUS_OR_RETURN(status, "OpenVolume");

        EFI_FILE_PROTOCOL *file = NULL;
        status = root->Open(
                root,
                &file,
                kernel_name,
                EFI_FILE_MODE_READ,
                EFI_FILE_READ_ONLY|EFI_FILE_HIDDEN|EFI_FILE_SYSTEM);

        if (!EFI_ERROR(status)) {
            void *buffer = NULL;
            EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
            UINTN buffer_size = sizeof(EFI_FILE_INFO) + sizeof(kernel_name);

            status = ST->BootServices->AllocatePool(EfiLoaderCode, buffer_size, &buffer);
            CHECK_EFI_STATUS_OR_RETURN(status, "Allocating kernel file info memory");

            status = file->GetInfo(file, &file_info_guid, &buffer_size, buffer);
            CHECK_EFI_STATUS_OR_RETURN(status, "Getting kernel file info");

            buffer_size = ((EFI_FILE_INFO*)buffer)->FileSize;

            status = ST->BootServices->FreePool(buffer);
            CHECK_EFI_STATUS_OR_RETURN(status, "Freeing kernel file info memory");

            UINTN page_count = ((buffer_size - 1) / 0x1000) + 1;
            EFI_PHYSICAL_ADDRESS addr = 0x100000; // Try to load the kernel in at 1MiB
            EFI_MEMORY_TYPE mem_type = 0xFFFFFFFF;  // Special value to tell the kernel it's here
            status = ST->BootServices->AllocatePages(AllocateAddress, mem_type, page_count, &addr);
            if (status == EFI_NOT_FOUND) {
                // couldn't get the address we wanted, try loading the kernel anywhere
                status = ST->BootServices->AllocatePages(AllocateAnyPages, mem_type, page_count, &addr);
            }
            CHECK_EFI_STATUS_OR_RETURN(status, "Allocating kernel pages");

            buffer = (void*)addr;
            status = file->Read(file, &buffer_size, buffer);
            CHECK_EFI_STATUS_OR_RETURN(status, "Reading kernel file");

            status = file->Close(file);
            CHECK_EFI_STATUS_OR_RETURN(status, "Closing kernel file handle");

            *length = buffer_size;
            *kernel_image = buffer;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}
