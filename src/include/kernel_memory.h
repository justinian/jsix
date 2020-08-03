#pragma once
/// \file kernel_memory.h
/// Constants related to the kernel's memory layout

#include <stddef.h>
#include <stdint.h>

namespace memory {

	/// Size of a single page frame.
	constexpr size_t frame_size = 0x1000;

	/// Start of kernel memory.
	constexpr uintptr_t kernel_offset = 0xffff800000000000;

	/// Offset from physical where page tables are mapped.
	constexpr uintptr_t page_offset = 0xffffc00000000000;

	/// Number of pages for a kernel stack
	constexpr unsigned kernel_stack_pages = 1;

	/// Max size of the kernel heap
	constexpr size_t kernel_max_heap = 0x8000000000; // 512GiB

	/// Start of the kernel heap
	constexpr uintptr_t heap_start = page_offset - kernel_max_heap;

	/// Start of the kernel stacks
	constexpr uintptr_t stacks_start = heap_start - kernel_max_heap;

	/// Max size of the kernel stacks area
	constexpr size_t kernel_max_stacks = 0x8000000000; // 512GiB

	/// First kernel space PML4 entry
	constexpr unsigned pml4e_kernel = 256;

	/// First offset-mapped space PML4 entry
	constexpr unsigned pml4e_offset = 384;

	/// Number of page_table entries
	constexpr unsigned table_entries = 512;

	/// Helper to determine if a physical address can be accessed
	/// through the page_offset area.
	inline bool page_mappable(uintptr_t a) { return (a & page_offset) == 0; }

} // namespace memory
