#pragma once
#include <efi.h>

#ifndef KERNEL_FILENAME
#define KERNEL_FILENAME L"kernel.bin"
#endif

EFI_STATUS loader_load_kernel(void **kernel_image, UINT64 *length);
