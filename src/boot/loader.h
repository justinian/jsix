#pragma once
#include <efi.h>

#ifndef KERNEL_PHYS_ADDRESS
#define KERNEL_PHYS_ADDRESS 0x0000000000100000
#endif

#ifndef KERNEL_VIRT_ADDRESS
#define KERNEL_VIRT_ADDRESS 0x7fff000000000000
#endif

#ifndef KERNEL_MEMTYPE
#define KERNEL_MEMTYPE 0xffffffff
#endif

#ifndef KERNEL_FILENAME
#define KERNEL_FILENAME L"kernel.bin"
#endif

EFI_STATUS loader_load_kernel(void **kernel_image, UINT64 *length);
