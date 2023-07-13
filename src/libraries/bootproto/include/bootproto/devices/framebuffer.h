#pragma once
/// \file bootproto/devices/uefi_fb.h
/// Data structures describing bootloader-passed framebuffer

#include <util/counted.h>
#include <util/hash.h>

namespace bootproto {
namespace devices {

inline constexpr uint64_t type_id_uefi_fb = "uefi.fb"_id;

enum class fb_layout : uint8_t { rgb8, bgr8, unknown = 0xff };

struct video_mode
{
    uint32_t vertical;
    uint32_t horizontal;
    uint32_t scanline;
    fb_layout layout;
};

struct uefi_fb
{
    util::buffer framebuffer;
    video_mode mode;
};


} // namespace devices
} // namespace bootproto
