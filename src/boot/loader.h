/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

#include <uefi/boot_services.h>

#include "kernel_args.h"
#include "memory.h"
#include "types.h"

namespace boot {

namespace fs { class file; }

namespace loader {

/// Load a file from disk into memory.
/// \arg disk  The opened UEFI filesystem to load from
/// \arg name  Name of the module (informational only)
/// \arg path  Path on `disk` of the file to load
/// \arg type  Memory type to use for allocation
buffer
load_file(
	fs::file &disk,
	const wchar_t *name,
	const wchar_t *path,
	uefi::memory_type type = uefi::memory_type::loader_data);

/// Parse and load an ELF file in memory into a loaded image.
/// \arg program  The program structure to fill
/// \arg name     The name of the program being loaded
/// \arg data     Buffer of the ELF file in memory
/// \arg bs       Boot services
void
load_program(
	kernel::init::program &program,
	const wchar_t *name,
	buffer data,
	uefi::boot_services *bs);

} // namespace loader
} // namespace boot
