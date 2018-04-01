#pragma once
#include <efi.h>

#ifndef KERNEL_PHYS_ADDRESS
#define KERNEL_PHYS_ADDRESS 0x100000
#endif

#ifndef VIRTUAL_OFFSET
#define VIRTUAL_OFFSET 0xf00000000
#endif

#ifndef KERNEL_MEMTYPE
#define KERNEL_MEMTYPE 0x80000000
#endif

#ifndef KERNEL_DATA_MEMTYPE
#define KERNEL_DATA_MEMTYPE 0x80000001
#endif

#ifndef KERNEL_DATA_PAGES
#define KERNEL_DATA_PAGES 1
#endif

#ifndef KERNEL_FILENAME
#define KERNEL_FILENAME L"kernel.bin"
#endif

EFI_STATUS loader_load_kernel(
	EFI_BOOT_SERVICES *bootsvc,
	void **kernel_image,
	uint64_t *kernel_length,
	void **kernel_data,
	uint64_t *data_length);
