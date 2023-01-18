#include <uefi/boot_services.h>
#include <uefi/graphics.h>
#include <uefi/protos/graphics_output.h>

#include <bootproto/init.h>

#include "allocator.h"
#include "console.h"
#include "error.h"
#include "video.h"

namespace boot {
namespace video {

using bootproto::devices::fb_layout;
using bootproto::module_type;

static uefi::protos::graphics_output *
get_gop(uefi::boot_services *bs)
{
    uefi::protos::graphics_output *gop = nullptr;
    uefi::guid guid = uefi::protos::graphics_output::guid;

    uefi::status has_gop = bs->locate_protocol(&guid, nullptr,
        (void **)&gop);

    if (has_gop != uefi::status::success)
        return nullptr;

    return gop;
}

screen *
pick_mode(uefi::boot_services *bs)
{
    uefi::protos::graphics_output *gop = get_gop(bs);
    if (!gop) {
        console::print(L"No framebuffer found.\r\n");
        return nullptr;
    }

    uefi::graphics_output_mode_info *info = gop->mode->info;

    uint32_t best = gop->mode->mode;
    uint32_t res = info->horizontal_resolution * info->vertical_resolution;
    int pixmode = static_cast<int>(info->pixel_format);

    const uint32_t modes = gop->mode->max_mode;
    for (uint32_t i = 0; i < modes; ++i) {
        size_t size = 0;
        uefi::graphics_output_mode_info *new_info = nullptr;

        try_or_raise(
            gop->query_mode(i, &size, &new_info),
            L"Failed to find a graphics mode the driver claimed to support");

        const uint32_t new_res = new_info->horizontal_resolution * new_info->vertical_resolution;
        int new_pixmode = static_cast<int>(new_info->pixel_format);

        if (new_pixmode <= pixmode && new_res >= res) {
            best = i;
            res = new_res;
            pixmode = new_pixmode;
        }
    }

    screen *s = new screen;
    s->mode = {
        .vertical = gop->mode->info->vertical_resolution,
        .horizontal = gop->mode->info->horizontal_resolution,
        .scanline =  gop->mode->info->pixels_per_scanline,
        .layout = fb_layout::unknown,
    };

    s->framebuffer = {
        .pointer = reinterpret_cast<void*>(gop->mode->frame_buffer_base),
        .count = gop->mode->frame_buffer_size
    };

    wchar_t const * type = nullptr;
    switch (info->pixel_format) {
        case uefi::pixel_format::rgb8:
            type = L"rgb8";
            s->mode.layout = fb_layout::rgb8;
            break;

        case uefi::pixel_format::bgr8:
            type = L"bgr8";
            s->mode.layout = fb_layout::bgr8;
            break;

        default:
            type = L"unknown";
    }

    console::print(L"Found framebuffer: %dx%d[%d] type %s @0x%x\r\n",
        info->horizontal_resolution, info->vertical_resolution,
        info->pixels_per_scanline, type, gop->mode->frame_buffer_base);

    try_or_raise(
        gop->set_mode(best),
        L"Failed to set graphics mode");

    return s;
}

void
make_module(screen *s)
{
    using bootproto::module;
    using bootproto::module_type;
    using bootproto::device_type;
    using bootproto::devices::uefi_fb;

    uefi_fb *fb = new uefi_fb;
    fb->framebuffer = s->framebuffer;
    fb->mode = s->mode;

    module *mod = g_alloc.allocate_module();
    mod->type = module_type::device;
    mod->subtype = static_cast<uint16_t>(device_type::uefi_fb);
    mod->data = {
        .pointer = fb,
        .count = sizeof(uefi_fb),
    };
}

} // namespace video
} // namespace boot
