#pragma once
#include <efi.h>

struct memory_map {
	size_t length;
	size_t size;
	size_t key;
	uint32_t version;
	EFI_MEMORY_DESCRIPTOR *entries;
};

EFI_STATUS memory_mark_address_for_update(void **pointer);

EFI_STATUS memory_virtualize(EFI_MEMORY_DESCRIPTOR *memory_map, size_t memmap_size, size_t desc_size,
							 UINT32 desc_version);

EFI_STATUS memory_get_map_size(size_t *sizsize);

EFI_STATUS memory_get_map(EFI_MEMORY_DESCRIPTOR **buffer, size_t *buffer_size, size_t *key,
						  size_t *desc_size, UINT32 *desc_version);

EFI_STATUS memory_copy_map(EFI_MEMORY_DESCRIPTOR *oldmap, EFI_MEMORY_DESCRIPTOR *newmap,
						   size_t oldmap_size, size_t desc_size, void *this_image,
						   size_t *newmap_size);

EFI_STATUS memory_dump_map();
