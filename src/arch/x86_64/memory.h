#pragma once
#include <efi.h>

const CHAR16 *memory_type_name(UINT32 value);

EFI_STATUS get_memory_map(
    EFI_MEMORY_DESCRIPTOR **buffer,
    UINTN *buffer_size,
    UINTN *key,
    UINTN *desc_size,
    UINT32 *desc_version);

EFI_STATUS dump_memory_map();