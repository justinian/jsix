#pragma once
#include <efi.h>

struct memory_map {
	size_t length;
	size_t size;
	size_t key;
	uint32_t version;
	EFI_MEMORY_DESCRIPTOR *entries;
};

EFI_STATUS memory_get_map_length(size_t *size);
EFI_STATUS memory_get_map(struct memory_map *map);
EFI_STATUS memory_dump_map(struct memory_map *map);
