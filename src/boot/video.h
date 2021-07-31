#pragma once
/// \file video.h
/// Video mode handling

#include <stdarg.h>
#include <stddef.h>

#include "init_args.h"

namespace uefi {
	struct boot_services;
}

namespace boot {
namespace video {

using kernel::init::video_mode;
using layout = kernel::init::fb_layout;

struct screen {
	buffer framebuffer;
	video_mode mode;
};

/// Pick the best video mode and set up the screen
screen * pick_mode(uefi::boot_services *bs);

/// Make an init arg module from the video mode
void make_module(screen *s);

} // namespace video
} // namespace boot
