#pragma once
#include <efi.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000

#ifndef KERNEL_PHYS_ADDRESS
#define KERNEL_PHYS_ADDRESS 0x100000
#endif

#ifndef KERNEL_VIRT_ADDRESS
#define KERNEL_VIRT_ADDRESS 0xFFFF800000000000
#endif

#ifndef VIRTUAL_OFFSET
#define VIRTUAL_OFFSET 0xf00000000
#endif

#ifndef KERNEL_MEMTYPE
#define KERNEL_MEMTYPE 0x80000000
#endif

#ifndef KERNEL_FONT_MEMTYPE
#define KERNEL_FONT_MEMTYPE 0x80000001
#endif

#ifndef KERNEL_DATA_MEMTYPE
#define KERNEL_DATA_MEMTYPE 0x80000002
#endif

#ifndef KERNEL_PT_MEMTYPE
#define KERNEL_PT_MEMTYPE 0x80000004
#endif

#ifndef KERNEL_FILENAME
#define KERNEL_FILENAME L"kernel.elf"
#endif

#ifndef KERNEL_FONT
#define KERNEL_FONT L"screenfont.psf"
#endif

struct loader_data {
	void *kernel;
	void *kernel_entry;
	size_t kernel_length;

	void *font;
	size_t font_length;

	void *data;
	size_t data_length;
};

EFI_STATUS loader_load_kernel(EFI_BOOT_SERVICES *bootsvc, struct loader_data *data);
