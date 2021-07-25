/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

#include "counted.h"

namespace kernel {
namespace init {
	struct program;
}}

namespace boot {

namespace fs {
	class file;
}

namespace loader {

/// Load a file from disk into memory.
/// \arg disk  The opened UEFI filesystem to load from
/// \arg name  Name of the module (informational only)
/// \arg path  Path on `disk` of the file to load
buffer
load_file(
	fs::file &disk,
	const wchar_t *name,
	const wchar_t *path);

/// Parse and load an ELF file in memory into a loaded image.
/// \arg program  The program structure to fill
/// \arg name     The name of the program being loaded
/// \arg data     Buffer of the ELF file in memory
void
load_program(
	kernel::init::program &program,
	const wchar_t *name,
	buffer data);

/// Verify that a loaded ELF has the j6 kernel header
/// \arg program  The program to check for a header
void
verify_kernel_header(kernel::init::program &program);

} // namespace loader
} // namespace boot
