#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <j6/init.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/syscalls.h>
#include <j6/types.h>

#include "font.h"
#include "screen.h"
#include "scrollback.h"

extern "C" {
    int main(int, const char **);
    void _get_init(size_t *initc, struct j6_init_value **initv);
}

extern j6_handle_t __handle_sys;
extern j6_handle_t __handle_self;

struct entry
{
    uint8_t bytes;
    uint8_t area;
    uint8_t severity;
    uint8_t sequence;
    char message[0];
};

int
main(int argc, const char **argv)
{
    j6_log("fb driver starting");

    size_t initc = 0;
    j6_init_value *initv = nullptr;

    j6_init_framebuffer *fb = nullptr;
    for (unsigned i = 0; i < initc; ++i) {
        if (initv[i].type == j6_init_desc_framebuffer) {
            fb = reinterpret_cast<j6_init_framebuffer*>(initv[i].data);
            break;
        }
    }

    if (!fb || fb->addr == 0) {
        j6_log("fb driver didn't find a framebuffer, exiting");
        return 1;
    }

    j6_handle_t fb_handle = j6_handle_invalid;
    uint32_t flags =
        j6_vm_flag_write |
        j6_vm_flag_write_combine |
        j6_vm_flag_mmio;
    j6_status_t s = j6_system_map_phys(__handle_sys, &fb_handle, fb->addr, fb->size, flags);
    if (s != j6_status_ok) {
        return s;
    }

    s = j6_vma_map(fb_handle, __handle_self, fb->addr);
    if (s != j6_status_ok) {
        return s;
    }

    const screen::pixel_order order = (fb->flags & 1) ?
        screen::pixel_order::bgr8 : screen::pixel_order::rgb8;

    screen scr(
        reinterpret_cast<void*>(fb->addr),
        fb->horizontal,
        fb->vertical,
        fb->scanline,
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

    j6_handle_t sys = __handle_sys;
    size_t buffer_size = 0;
    void *message_buffer = nullptr;

    while (true) {
        size_t size = buffer_size;
        j6_status_t s = j6_system_get_log(sys, message_buffer, &size);

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
            entry *e = reinterpret_cast<entry*>(message_buffer);

            size_t eom = e->bytes - sizeof(entry);
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

