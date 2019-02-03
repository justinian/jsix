#pragma once
#include <efi/efi.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000

#ifndef KERNEL_PHYS_ADDRESS
#define KERNEL_PHYS_ADDRESS 0x100000
#endif

#ifndef KERNEL_VIRT_ADDRESS
#define KERNEL_VIRT_ADDRESS 0xFFFFFF0000000000
#endif

#ifndef KERNEL_MEMTYPE
#define KERNEL_MEMTYPE static_cast<EFI_MEMORY_TYPE>(0x80000000)
#endif

#ifndef INITRD_MEMTYPE
#define INITRD_MEMTYPE static_cast<EFI_MEMORY_TYPE>(0x80000001)
#endif

#ifndef KERNEL_DATA_MEMTYPE
#define KERNEL_DATA_MEMTYPE static_cast<EFI_MEMORY_TYPE>(0x80000002)
#endif

#ifndef KERNEL_FILENAME
#define KERNEL_FILENAME L"kernel.elf"
#endif

#ifndef INITRD_FILENAME
#define INITRD_FILENAME L"initrd.img"
#endif

struct loader_data {
	void *kernel;
	void *kernel_entry;
	size_t kernel_length;

	void *initrd;
	size_t initrd_length;

	void *data;
	size_t data_length;
};

EFI_STATUS loader_load_kernel(EFI_BOOT_SERVICES *bootsvc, struct loader_data *data);
