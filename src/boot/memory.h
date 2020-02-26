#pragma once
#include <uefi/boot_services.h>
#include <uefi/runtime_services.h>
#include <stdint.h>
#include "kernel_args.h"

namespace boot {
namespace memory {

constexpr size_t page_size = 0x1000;

inline constexpr size_t bytes_to_pages(size_t bytes) {
	return ((bytes - 1) / page_size) + 1;
}

void init_pointer_fixup(uefi::boot_services *bs, uefi::runtime_services *rs);
void mark_pointer_fixup(void **p);

kernel::args::header * allocate_args_structure(uefi::boot_services *bs, size_t max_modules);

/*
extern const EFI_MEMORY_TYPE memtype_kernel;
extern const EFI_MEMORY_TYPE memtype_data;
extern const EFI_MEMORY_TYPE memtype_initrd;
extern const EFI_MEMORY_TYPE memtype_scratch;

struct memory_map {
	size_t length;
	size_t size;
	size_t key;
	uint32_t version;
	EFI_MEMORY_DESCRIPTOR *entries;
};

EFI_STATUS memory_get_map_length(EFI_BOOT_SERVICES *bootsvc, size_t *size);
EFI_STATUS memory_get_map(EFI_BOOT_SERVICES *bootsvc, struct memory_map *map);
EFI_STATUS memory_dump_map(struct memory_map *map);

void memory_virtualize(EFI_RUNTIME_SERVICES *runsvc, struct memory_map *map);
*/

} // namespace boot
} // namespace memory
