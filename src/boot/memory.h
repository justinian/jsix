#pragma once
/// \file memory.h
/// Memory-related constants and functions.

#include <stdint.h>

namespace uefi {
	struct boot_services;
	struct runtime_services;
}

namespace boot {
namespace memory {

class efi_mem_map;

/// UEFI specifies that pages are always 4 KiB.
constexpr size_t page_size = 0x1000;

/// Get the number of pages needed to hold `bytes` bytes
inline constexpr size_t bytes_to_pages(size_t bytes) {
	return ((bytes - 1) / page_size) + 1;
}

/// \defgroup pointer_fixup
/// Memory virtualization pointer fixup functions. Handles changing affected pointers
/// when calling UEFI's `set_virtual_address_map` function to change the location of
/// runtime services in virtual memory.
/// @{

/// Set up the pointer fixup UEFI events. This registers the necessary callbacks for
/// runtime services to call when `set_virtual_address_map` is called.
void init_pointer_fixup(uefi::boot_services *bs, uefi::runtime_services *rs);

/// Mark a given pointer as needing to be updated when doing pointer fixup.
void mark_pointer_fixup(void **p);

/// @}

/// Activate the given memory mappings. Sets the given page tables live as well
/// as informs UEFI runtime services of the new mappings.
/// \arg pml4  The root page table for the new mappings
/// \arg map   The UEFI memory map, used to update runtime services
void virtualize(
	void *pml4,
	efi_mem_map &map,
	uefi::runtime_services *rs);

} // namespace boot
} // namespace memory
