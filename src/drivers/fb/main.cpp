#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "j6/init.h"
#include "j6/errors.h"
#include "j6/signals.h"
#include "j6/types.h"

#include <j6libc/syscalls.h>

#include "font.h"
#include "screen.h"
#include "scrollback.h"

extern "C" {
	int main(int, const char **);
	void _get_init(size_t *initc, struct j6_init_value **initv);
}

extern j6_handle_t __handle_sys;

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
	_syscall_system_log("fb driver starting");

	size_t initc = 0;
	j6_init_value *initv = nullptr;
	_get_init(&initc, &initv);

	j6_init_framebuffer *fb = nullptr;
	for (unsigned i = 0; i < initc; ++i) {
		if (initv[i].type == j6_init_desc_framebuffer) {
			fb = reinterpret_cast<j6_init_framebuffer*>(initv[i].value);
			break;
		}
	}

	if (!fb || fb->addr == nullptr) {
		_syscall_system_log("fb driver didn't find a framebuffer, exiting");
		return 1;
	}

	const screen::pixel_order order = (fb->flags & 1) ?
		screen::pixel_order::bgr8 : screen::pixel_order::rgb8;

	screen scr(fb->addr, fb->horizontal, fb->vertical, order);
	font fnt;

	screen::pixel_t fg = scr.color(0xb0, 0xb0, 0xb0);
	screen::pixel_t bg = scr.color(49, 79, 128);
	scr.fill(bg);

	constexpr int margin = 2;
	const unsigned xstride = (margin + fnt.width());
	const unsigned ystride = (margin + fnt.height());
	const unsigned rows = (scr.height() - margin) / ystride;
	const unsigned cols = (scr.width() - margin) / xstride;

	scrollback scroll(rows, cols);

	int pending = 0;
	constexpr int pending_threshold = 10;

	char message_buffer[256];
	while (true) {
		size_t size = sizeof(message_buffer);
		_syscall_system_get_log(__handle_sys, message_buffer, &size);
		if (size != 0) {
			entry *e = reinterpret_cast<entry*>(&message_buffer);

			size_t eom = e->bytes - sizeof(entry);
			e->message[eom] = 0;

			scroll.add_line(e->message, eom);
			if (++pending > pending_threshold) {
				scroll.render(scr, fnt);
				pending = 0;
			}
		} else {
			if (pending) {
				scroll.render(scr, fnt);
				pending = 0;
			}
		}
	}


	_syscall_system_log("fb driver done, exiting");
	return 0;
}

