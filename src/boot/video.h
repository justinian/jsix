#pragma once
/// \file video.h
/// Video mode handling

#include <stdarg.h>
#include <stddef.h>

#include <bootproto/init.h>
#include <util/counted.h>

namespace uefi {
    struct boot_services;
}

namespace boot {
namespace video {

using bootproto::video_mode;
using layout = bootproto::fb_layout;

struct screen {
    util::buffer framebuffer;
    video_mode mode;
};

/// Pick the best video mode and set up the screen
screen * pick_mode(uefi::boot_services *bs);

/// Make an init arg module from the video mode
void make_module(screen *s);

} // namespace video
} // namespace boot
