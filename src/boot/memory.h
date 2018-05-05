#pragma once
#include <efi/efi.h>

struct memory_map {
	size_t length;
	size_t size;
	size_t key;
	uint32_t version;
	EFI_MEMORY_DESCRIPTOR *entries;
};

EFI_STATUS memory_init_pointer_fixup(EFI_BOOT_SERVICES *bootsvc, EFI_RUNTIME_SERVICES *runsvc);
void memory_mark_pointer_fixup(void **p);

EFI_STATUS memory_get_map_length(EFI_BOOT_SERVICES *bootsvc, size_t *size);
EFI_STATUS memory_get_map(EFI_BOOT_SERVICES *bootsvc, struct memory_map *map);
EFI_STATUS memory_dump_map(struct memory_map *map);

void memory_virtualize(EFI_RUNTIME_SERVICES *runsvc, struct memory_map *map);
