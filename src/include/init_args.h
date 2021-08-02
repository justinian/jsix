#pragma once

#include <stddef.h>
#include <stdint.h>

#include "counted.h"

namespace kernel {
namespace init {

enum class module_type : uint8_t {
    none,
    program,
    framebuffer,
};

enum class module_flags : uint8_t {
    none      = 0x00,

    /// This module was already handled by the bootloader,
    /// no action is needed. The module is included for
    /// informational purposes only.
    no_load   = 0x01,
};

struct module
{
    module_type mod_type;
    module_flags mod_flags;
    uint32_t mod_length;
};

struct module_program :
    public module
{
    uintptr_t base_address;
    char filename[];
};

enum class fb_layout : uint8_t { rgb8, bgr8, unknown = 0xff };
enum class fb_type : uint8_t { uefi };

struct video_mode
{
    uint32_t vertical;
    uint32_t horizontal;
    uint32_t scanline;
    fb_layout layout;
};

struct module_framebuffer :
    public module
{
    buffer framebuffer;
    video_mode mode;
    fb_type type;
};

struct modules_page
{
    uint8_t count;
    module *modules;
    uintptr_t next;
};

} // namespace init
} // namespace kernel
