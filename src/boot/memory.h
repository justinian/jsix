#pragma once
#include <efi.h>

EFI_STATUS memory_mark_address_for_update(void **pointer);

EFI_STATUS memory_virtualize(EFI_MEMORY_DESCRIPTOR *memory_map, UINTN memmap_size, UINTN desc_size,
							 UINT32 desc_version);

EFI_STATUS memory_get_map(EFI_MEMORY_DESCRIPTOR **buffer, UINTN *buffer_size, UINTN *key,
						  UINTN *desc_size, UINT32 *desc_version);

EFI_STATUS memory_dump_map();
