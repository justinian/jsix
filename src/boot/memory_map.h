#pragma once
/// \file memory_map.h
/// Memory-map related types and functions

#include <util/counted.h>
#include <util/pointers.h>

namespace uefi {
    struct boot_services;
    struct memory_descriptor;
}

namespace bootproto {
    struct args;
    struct frame_block;
    struct mem_entry;
}

namespace boot {

namespace paging {
    class pager;
}

namespace memory {

/// Struct that represents UEFI's memory map. Contains a pointer to the map data
/// as well as the data on how to read it.
struct efi_mem_map
{
    using desc = uefi::memory_descriptor;
    using iterator = util::offset_iterator<desc>;

    size_t length;     ///< Total length of the map data
    size_t total;      ///< Total allocated space for map data
    size_t size;       ///< Size of an entry in the array
    size_t key;        ///< Key for detecting changes
    uint32_t version;  ///< Version of the `memory_descriptor` struct
    desc *entries;     ///< The array of UEFI descriptors

    efi_mem_map() : length(0), total(0), size(0), key(0), version(0), entries(nullptr) {}

    /// Update the map from UEFI
    void update(uefi::boot_services &bs);

    /// Get the count of entries in the array
    inline size_t num_entries() const { return length / size; }

    /// Return an iterator to the beginning of the array
    inline iterator begin() { return iterator(entries, size); }

    /// Return an iterator to the end of the array
    inline iterator end() { return util::offset_pointer(entries, length); }
};

/// Add the kernel's memory map as a module to the kernel args.
/// \returns  The uefi memory map used to build the kernel map
util::counted<bootproto::mem_entry> build_kernel_map(efi_mem_map &map);

/// Create the kernel frame allocation maps
util::counted<bootproto::frame_block> build_frame_blocks(const util::counted<bootproto::mem_entry> &kmap);

/// Map the frame allocation maps to the right spot and fix up pointers
void fix_frame_blocks(bootproto::args *args, paging::pager &pager);

} // namespace boot
} // namespace memory
