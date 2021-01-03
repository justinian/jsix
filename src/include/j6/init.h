#pragma once
/// \file init.h
/// Types used in process and thread initialization

#include <stdint.h>

enum j6_init_type {					// `value` is a:
	j6_init_handle_system,			// Handle to the system
	j6_init_handle_process,			// Handle to this process
	j6_init_handle_thread,			// Handle to this thread
	j6_init_handle_space,			// Handle to this process' address space
	j6_init_desc_framebuffer		// Pointer to a j6_init_framebuffer descriptor
};

struct j6_init_value {
	enum j6_init_type type;
	uint64_t value;
};

/// Structure defining a framebuffer.
/// `flags` has the following bits:
///   0-3:   Pixel layout. 0000: rgb8, 0001: bgr8
struct j6_init_framebuffer {
	void* addr;
	size_t size;
	uint32_t vertical;
	uint32_t horizontal;
	uint32_t scanline;
	uint32_t flags;
};
