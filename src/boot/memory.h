#pragma once
#include <efi.h>

EFI_STATUS memory_virtualize();

EFI_STATUS memory_get_map(
    EFI_MEMORY_DESCRIPTOR **buffer,
    UINTN *buffer_size,
    UINTN *key,
    UINTN *desc_size,
    UINT32 *desc_version);

EFI_STATUS memory_dump_map();
