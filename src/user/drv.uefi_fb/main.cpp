#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <bootproto/devices/framebuffer.h>

#include <j6/init.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/syscalls.h>
#include <j6/types.h>

#include "font.h"
#include "screen.h"
#include "scrollback.h"

extern "C" {
    void _get_init(size_t *initc, struct j6_init_value **initv);
}

int
driver_main(unsigned argc, const char **argv, const char **env, const j6_init_args *init)
{
    j6_log("fb driver starting");

    using bootproto::devices::uefi_fb;
    using bootproto::devices::video_mode;
    using bootproto::devices::fb_layout;

    const uefi_fb *fb = reinterpret_cast<const uefi_fb*>(init->args[0]);

    if (!fb || !fb->framebuffer) {
        j6_log("fb driver didn't find a framebuffer, exiting");
        return 1;
    }

    util::buffer lfb = fb->framebuffer;
    const video_mode &mode = fb->mode;

    j6_handle_t fb_handle = j6_handle_invalid;
    uint32_t flags =
        j6_vm_flag_write |
        j6_vm_flag_write_combine |
        j6_vm_flag_mmio;

    j6_handle_t sys = j6_find_first_handle(j6_object_type_system);
    if (sys == j6_handle_invalid)
        return 1;

    uintptr_t lfb_addr = reinterpret_cast<uintptr_t>(lfb.pointer);
    j6_status_t s = j6_system_map_phys( sys, &fb_handle, lfb_addr, lfb.count, flags);
    if (s != j6_status_ok) {
        return s;
    }

    s = j6_vma_map(fb_handle, 0, lfb_addr);
    if (s != j6_status_ok) {
        return s;
    }

    const screen::pixel_order order =
        (mode.layout == fb_layout::bgr8) ?
            screen::pixel_order::bgr8 : screen::pixel_order::rgb8;

    screen scr(
        reinterpret_cast<void*>(lfb.pointer),
        mode.horizontal,
        mode.vertical,
        mode.scanline,
        order);

    font fnt;

    screen::pixel_t fg = scr.color(0xb0, 0xb0, 0xb0);
    screen::pixel_t bg = scr.color(49, 79, 128);
    scr.fill(bg);
    scr.update();

    constexpr int margin = 2;
    const unsigned xstride = (margin + fnt.width());
    const unsigned ystride = (margin + fnt.height());
    const unsigned rows = (scr.height() - margin) / ystride;
    const unsigned cols = (scr.width() - margin) / xstride;

    scrollback scroll(rows, cols);

    int pending = 0;
    constexpr int pending_threshold = 5;

    size_t buffer_size = 0;
    void *message_buffer = nullptr;

    uint64_t seen = 0;

    while (true) {
        size_t size = buffer_size;
        j6_status_t s = j6_system_get_log(sys, seen, message_buffer, &size);

        if (s == j6_err_insufficient) {
            free(message_buffer);
            message_buffer = malloc(size * 2);
            buffer_size = size;
            continue;
        } else if (s != j6_status_ok) {
            j6_log("fb driver got error from get_log, quitting");
            return s;
        }

        if (size > 0) {
            j6_log_entry *e = reinterpret_cast<j6_log_entry*>(message_buffer);

            size_t eom = e->bytes - sizeof(j6_log_entry);
            e->message[eom] = 0;

            scroll.add_line(e->message, eom);
            if (++pending > pending_threshold) {
                scroll.render(scr, fnt);
                scr.update();
                pending = 0;
            }
        } else {
            if (pending) {
                scroll.render(scr, fnt);
                scr.update();
                pending = 0;
            }
        }
    }

    j6_log("fb driver done, exiting");
    return 0;
}
