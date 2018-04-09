#include <stddef.h>
#include <stdint.h>

#include "font.h"
#include "kernel_data.h"
#include "screen.h"

extern "C" {
    void do_the_set_registers(popcorn_data *header);
    void kernel_main(popcorn_data *header);
}

void
kernel_main(popcorn_data *header)
{
	font f = font::load(header->font);
	screen s{
        header->frame_buffer,
        header->hres,
        header->vres,
        header->rmask,
        header->gmask,
        header->bmask};

	uint32_t color = header->gmask;
	uint32_t perline = header->hres / f.width();
	const char message[] = "Hello, I am text rendered by the kernel! :-D  ";
	for (int i=0; i<sizeof(message); ++i) {
        f.draw_glyph(s, message[i], color,
			(i % perline) * f.width() + 10,
			(i / perline) * f.height() + 10);
	}

	do_the_set_registers(header);
}
