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
	struct screen screen;
	struct font font;
	int status = 0;

	status = font_init(header->font, &font);

	status = screen_init(
		header->frame_buffer,
		header->hres,
		header->vres,
		header->rmask,
		header->gmask,
		header->bmask,
		&screen);

	uint32_t color = header->gmask;
	uint32_t perline = header->hres / font.width;
	const char message[] = "Hello, I am text rendered by the kernel! :-D  ";
	for (int i=0; i<sizeof(message); ++i) {
		font_draw(
			&font,
			&screen,
			message[i],
			color,
			(i % perline) * font.width + 10,
			(i / perline) * font.height + 10);
	}

	do_the_set_registers(header);
}
