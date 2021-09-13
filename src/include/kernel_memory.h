#pragma once
/// \file kernel_memory.h
/// Constants related to the kernel's memory layout

#include <stddef.h>
#include <stdint.h>

namespace memory {

    /// Size of a single page frame.
    constexpr size_t frame_size = 0x1000;

    /// Number of bits of addressing within a page
    constexpr size_t frame_bits = 12;

    /// Start of kernel memory.
    constexpr uintptr_t kernel_offset = 0xffff800000000000ull;

    /// Offset from physical where page tables are mapped.
    constexpr uintptr_t page_offset = 0xffffc00000000000ull;

    /// Max number of pages for a kernel stack
    constexpr unsigned kernel_stack_pages = 2;

    /// Max number of pages for a kernel buffer
    constexpr unsigned kernel_buffer_pages = 16;

    /// Max size of the kernel heap
    constexpr size_t kernel_max_heap = 0x8000000000ull; // 512GiB

    /// Start of the kernel heap
    constexpr uintptr_t heap_start = page_offset - kernel_max_heap;

    /// Max size of the kernel stacks area
    constexpr size_t kernel_max_stacks = 0x8000000000ull; // 512GiB

    /// Start of the kernel stacks
    constexpr uintptr_t stacks_start = heap_start - kernel_max_stacks;

    /// Max size of kernel buffers area
    constexpr size_t kernel_max_buffers = 0x8000000000ull; // 512GiB

    /// Start of kernel buffers
    constexpr uintptr_t buffers_start = stacks_start - kernel_max_buffers;

    /// Max size of kernel bitmap area
    constexpr size_t kernel_max_bitmap = 0x8000000000ull; // 512GiB

    /// Start of kernel bitmap
    constexpr uintptr_t bitmap_start = buffers_start - kernel_max_bitmap;

    /// First kernel space PML4 entry
    constexpr unsigned pml4e_kernel = 256;

    /// First offset-mapped space PML4 entry
    constexpr unsigned pml4e_offset = 384;

    /// Number of page_table entries
    constexpr unsigned table_entries = 512;

    /// Helper to determine if a physical address can be accessed
    /// through the page_offset area.
    inline bool page_mappable(uintptr_t a) { return (a & page_offset) == 0; }

    /// Convert a physical address to a virtual one (in the offset-mapped area)
    template <typename T> T * to_virtual(uintptr_t a) {
        return reinterpret_cast<T*>(a|page_offset);
    }

    /// Convert a physical address to a virtual one (in the offset-mapped area)
    template <typename T> T * to_virtual(T *p) {
        return to_virtual<T>(reinterpret_cast<uintptr_t>(p));
    }

    /// Get the number of pages needed for a given number of bytes.
    /// \arg bytes  The number of bytes desired
    /// \returns    The number of pages needed to contain the desired bytes
    inline size_t page_count(size_t bytes) {
        return ((bytes - 1) >> 12) + 1;
    }

    /// Get the given address, aligned to the next lowest page
    inline uintptr_t page_align_down(uintptr_t a) { return a & ~(frame_size-1); }

    /// Get the given address, aligned to the next page
    inline uintptr_t page_align_up(uintptr_t a) { return page_align_down(a-1) + frame_size; }

} // namespace memory
