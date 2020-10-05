/// \file types.h
/// Definitions of shared types used throughout the bootloader
#pragma once

namespace boot {

struct buffer
{
	size_t size;
	void *data;
};

} // namespace boot
