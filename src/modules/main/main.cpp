#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "font.h"
#include "kernel_data.h"
#include "screen.h"

extern "C" {
	void do_the_set_registers(popcorn_data *header);
	void kernel_main(popcorn_data *header);
}

console
load_console(const popcorn_data *header)
{
	return console{
		font::load(header->font),
		screen{
			header->frame_buffer,
			header->hres,
			header->vres,
			header->rmask,
			header->gmask,
			header->bmask},
		header->log,
		header->log_length};
}

void
kernel_main(popcorn_data *header)
{
	console cons = load_console(header);

	const int times = 38;
	char message[] = " 000  Hello, I am text rendered by the kernel! :-D\n";
	for (int i = 0; i < times; ++i) {
		cons.set_color(i, 0);
		message[1] = '0' + ((i / 100) % 10);
		message[2] = '0' + ((i / 10) % 10);
		message[3] = '0' + (i % 10);
		cons.puts(message);
	}

	do_the_set_registers(header);
}
