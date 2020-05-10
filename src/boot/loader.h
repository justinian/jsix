/// \file loader.h
/// Definitions for loading the kernel into memory
#pragma once

namespace boot {
namespace loader {

/// Structure to hold information about loaded binary image.
struct loaded_elf
{
	void *data;           ///< Start of the kernel in memory
	uintptr_t vaddr;      ///< Virtual address to map to
	uintptr_t entrypoint; ///< (Virtual) address of the kernel entrypoint
};

/// Parse and load an ELF file in memory into a loaded image.
/// \arg data  The start of the ELF file in memory
/// \arg size  The size of the ELF file in memory
/// \returns   A `loaded_elf` structure defining the loaded image
loaded_elf load(const void *data, size_t size, uefi::boot_services *bs);

} // namespace loader
} // namespace boot
