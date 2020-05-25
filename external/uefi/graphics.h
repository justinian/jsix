#pragma once
#ifndef _uefi_graphics_h_
#define _uefi_graphics_h_

// This Source Code Form is part of the j6-uefi-headers project and is subject
// to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <stdint.h>
#include <uefi/types.h>

namespace uefi {

struct text_output_mode
{
	int32_t max_mode;
	int32_t mode;
	int32_t attribute;
	int32_t cursor_column;
	int32_t cursor_row;
	bool cursor_visible;
};

struct pixel_bitmask
{
	uint32_t red_mask;
	uint32_t green_mask;
	uint32_t blue_mask;
	uint32_t reserved_mask;
};

enum class pixel_format
{
	rgb8,
	bgr8,
	bitmask,
	blt_only
};

struct graphics_output_mode_info
{
	uint32_t version;
	uint32_t horizontal_resolution;
	uint32_t vertical_resolution;
	pixel_format pixel_format;
	pixel_bitmask pixel_information;
	uint32_t pixels_per_scanline;
};

struct graphics_output_mode
{
	uint32_t max_mode;
	uint32_t mode;
	graphics_output_mode_info *info;
	uint64_t size_of_info;
	uintptr_t frame_buffer_base;
	uint64_t frame_buffer_size;
};

enum class attribute : uint64_t
{
	black,
	blue,
	green,
	cyan,
	red,
	magenta,
	brown,
	light_gray,
	dark_gray,
	light_blue,
	light_green,
	light_cyan,
	light_red,
	light_magenta,
	yellow,
	white,

	background_black      = 0x00,
	background_blue       = 0x10,
	background_green      = 0x20,
	background_cyan       = 0x30,
	background_red        = 0x40,
	background_magenta    = 0x50,
	background_brown      = 0x60,
	background_light_gray = 0x70,
};

} // namespace uefi

#endif
