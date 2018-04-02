#pragma once
#include <efi.h>
#include <stddef.h>

#ifndef KERNEL_PHYS_ADDRESS
#define KERNEL_PHYS_ADDRESS 0x100000
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

#ifndef KERNEL_DATA_PAGES
#define KERNEL_DATA_PAGES 1
#endif

#ifndef KERNEL_FILENAME
#define KERNEL_FILENAME L"kernel.bin"
#endif

#ifndef KERNEL_FONT
#define KERNEL_FONT L"screenfont.psf"
#endif

struct loader_data {
	void *kernel_image;
	size_t kernel_image_length;

	void *kernel_data;
	size_t kernel_data_length;

	void *screen_font;
	size_t screen_font_length;
};

EFI_STATUS loader_load_kernel(EFI_BOOT_SERVICES *bootsvc, struct loader_data *data);
