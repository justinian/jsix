#pragma once
/// \file arch/amd64/memory.h
/// Architecture-specific constants related to memory and paging

#include <stdint.h>

namespace arch {

/// Size of a single page frame.
constexpr size_t frame_size = 0x1000;

/// Number of bits of addressing within a page
constexpr size_t frame_bits = 12;

/// Number of bits mapped per page table level
constexpr size_t table_bits = 9;

/// Number of page_table entries
constexpr unsigned table_entries = 512;

/// First index in the root page table for kernel space
constexpr unsigned kernel_root_index = table_entries / 2;

/// Number of page table levels
/// TODO: Add five-level paging support
constexpr unsigned page_table_levels = 4;

/// Start of kernel memory.
constexpr uintptr_t kernel_offset = 0ull - (1ull << (page_table_levels * table_bits + frame_bits - 1));

} // namespace arch
