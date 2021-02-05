#pragma once
/// \file init.h
/// Types used in process and thread initialization

#include <stdint.h>
#include "j6/types.h"

enum j6_init_type {					// `value` is a:
	j6_init_handle_self,			// Handle to the system
	j6_init_handle_other,			// Handle to this process
	j6_init_desc_framebuffer		// Pointer to a j6_init_framebuffer descriptor
};

struct j6_typed_handle {
	enum j6_object_type type;
	j6_handle_t handle;
};

struct j6_init_value {
	enum j6_init_type type;
	union {
		struct j6_typed_handle handle;
		void *data;
	};
};

/// Structure defining a framebuffer.
/// `flags` has the following bits:
///   0-3:   Pixel layout. 0000: rgb8, 0001: bgr8
struct j6_init_framebuffer {
	uintptr_t addr;
	size_t size;
	uint32_t vertical;
	uint32_t horizontal;
	uint32_t scanline;
	uint32_t flags;
};
