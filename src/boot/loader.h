/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

#include <uefi/boot_services.h>

#include "kernel_args.h"

namespace boot {
namespace loader {

/// Parse and load an ELF file in memory into a loaded image.
/// \arg data  The start of the ELF file in memory
/// \arg size  The size of the ELF file in memory
/// \arg args  The kernel args, used for modifying page tables
/// \returns   A descriptor defining the loaded image
kernel::entrypoint load(
	const void *data, size_t size,
	kernel::args::header *args,
	uefi::boot_services *bs);

} // namespace loader
} // namespace boot
