#pragma once
/// \file video.h
/// Video mode handling

#include <stdarg.h>
#include <stddef.h>

#include <bootproto/devices/framebuffer.h>
#include <util/counted.h>

namespace uefi {
    struct boot_services;
}

namespace boot {
namespace video {

using layout = bootproto::devices::fb_layout;
using screen = bootproto::devices::uefi_fb;

/// Pick the best video mode and set up the screen
screen * pick_mode(uefi::boot_services *bs);

/// Make an init arg module from the video mode
void make_module(screen *s);

} // namespace video
} // namespace boot
