#pragma once
/// \file kernel_memory.h
/// Constants related to the kernel's memory layout

#include <stddef.h>
#include <stdint.h>

namespace memory {

	/// Size of a single page frame.
	static const size_t frame_size = 0x1000;

	/// Start of kernel memory.
	static const uintptr_t kernel_offset = 0xffffff0000000000;

	/// Offset from physical where page tables are mapped.
	static const uintptr_t page_offset = 0xffffff8000000000;

	/// Initial process thread's stack address
	static const uintptr_t initial_stack = 0x0000800000000000;

	/// Initial process thread's stack size, in pages
	static const unsigned initial_stack_pages = 1;

	/// Max size of the kernel heap
	static const size_t kernel_max_heap = 0x800000000; // 32GiB

	/// Start of the kernel heap
	static const uintptr_t heap_start = page_offset - kernel_max_heap;

	/// Helper to determine if a physical address can be accessed
	/// through the page_offset area.
	inline bool page_mappable(uintptr_t a) { return (a & page_offset) == 0; }

} // namespace memory
