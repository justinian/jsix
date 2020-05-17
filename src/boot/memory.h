/// \file memory.h
/// Memory-related constants and functions.
#pragma once
#include <uefi/boot_services.h>
#include <uefi/runtime_services.h>
#include <stdint.h>
#include "kernel_args.h"
#include "pointer_manipulation.h"

namespace boot {
namespace memory {

/// UEFI specifies that pages are always 4 KiB.
constexpr size_t page_size = 0x1000;

/// Get the number of pages needed to hold `bytes` bytes
inline constexpr size_t bytes_to_pages(size_t bytes) {
	return ((bytes - 1) / page_size) + 1;
}

/// \defgroup memory_types
/// Custom UEFI memory type values used for data being passed to the kernel
/// @{

/// Memory containing the kernel args structure
constexpr uefi::memory_type args_type =
	static_cast<uefi::memory_type>(0x80000000);

/// Memory containing any loaded modules to be passed to the kernel
constexpr uefi::memory_type module_type =
	static_cast<uefi::memory_type>(0x80000001);

/// Memory containing loaded kernel code and data sections
constexpr uefi::memory_type kernel_type =
	static_cast<uefi::memory_type>(0x80000002);

/// Memory containing page tables set up by the loader
constexpr uefi::memory_type table_type =
	static_cast<uefi::memory_type>(0x80000003);

/// @}

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

/// Struct that represents UEFI's memory map. Contains a pointer to the map data
/// as well as the data on how to read it.
struct efi_mem_map
{
	using desc = uefi::memory_descriptor;
	using iterator = offset_iterator<desc>;

	size_t length;     ///< Total length of the map data
	size_t size;       ///< Size of an entry in the array
	size_t key;        ///< Key for detecting changes
	uint32_t version;  ///< Version of the `memory_descriptor` struct
	desc *entries;     ///< The array of UEFI descriptors

	efi_mem_map() : length(0), size(0), key(0), version(0), entries(nullptr) {}

	/// Get the count of entries in the array
	inline size_t num_entries() const { return length / size; }

	/// Return an iterator to the beginning of the array
	iterator begin() { return iterator(entries, size); }

	/// Return an iterator to the end of the array
	iterator end() { return offset_ptr<desc>(entries, length); }
};

/// Get the memory map from UEFI.
/// \arg allocate  If false, don't actually fetch the mappings, just
///                return a structure describing them.
efi_mem_map get_uefi_mappings(bool allocate, uefi::boot_services *bs);

/// Add the kernel's memory map as a module to the kernel args.
/// \returns  The uefi memory map key for the version used to build
///           this map
size_t build_kernel_mem_map(kernel::args::header *args, uefi::boot_services *bs);

} // namespace boot
} // namespace memory
