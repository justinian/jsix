#include <uefi/boot_services.h>
#include <uefi/graphics.h>
#include <uefi/protos/graphics_output.h>

#include "allocator.h"
#include "console.h"
#include "error.h"
#include "video.h"

namespace boot {
namespace video {

using bootproto::fb_layout;
using bootproto::fb_type;
using bootproto::module_flags;
using bootproto::module_framebuffer;
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
        .layout = layout::unknown,
    };

    s->framebuffer = {
        .pointer = reinterpret_cast<void*>(gop->mode->frame_buffer_base),
        .count = gop->mode->frame_buffer_size
    };

    wchar_t const * type = nullptr;
    switch (info->pixel_format) {
        case uefi::pixel_format::rgb8:
            type = L"rgb8";
            s->mode.layout = layout::rgb8;
            break;

        case uefi::pixel_format::bgr8:
            type = L"bgr8";
            s->mode.layout = layout::bgr8;
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
    using bootproto::module_framebuffer;
    module_framebuffer *modfb = g_alloc.allocate_module<module_framebuffer>();
    modfb->mod_type = module_type::framebuffer;
    modfb->type = fb_type::uefi; 

    modfb->framebuffer = s->framebuffer;
    modfb->mode = s->mode;
}

} // namespace video
} // namespace boot
