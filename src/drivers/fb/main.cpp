#include <stdint.h>
#include <stdlib.h>

#include "j6/init.h"
#include "j6/errors.h"
#include "j6/signals.h"
#include "j6/types.h"

#include <j6libc/syscalls.h>

#include "font.h"
#include "screen.h"

extern "C" {
	int main(int, const char **);
	void _get_init(size_t *initc, struct j6_init_value **initv);
}

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

	screen::pixel_t fg = scr.color(255, 255, 255);
	screen::pixel_t bg = scr.color(49, 79, 128);
	scr.fill(bg);

	constexpr int margin = 4;

	int y = margin;
	char g = 0;

	while (y < scr.height() - margin - fnt.height()) {
		int x = margin;
		while (x < scr.width() - margin - fnt.width()) {
			fnt.draw_glyph(scr, g+' ', fg, bg, x, y);
			x += fnt.width() + fnt.width() / 4;
			g = ++g % ('~' - ' ' + 1);
		}
		y += fnt.height() + fnt.height() / 4;
	}

	_syscall_system_log("fb driver done, exiting");
	return 0;
}

