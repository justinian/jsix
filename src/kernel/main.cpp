#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "device_manager.h"
#include "font.h"
#include "interrupts.h"
#include "kernel_data.h"
#include "memory.h"
#include "screen.h"

extern "C" {
	void do_the_set_registers(popcorn_data *header);
	void kernel_main(popcorn_data *header);
}

console
load_console(const popcorn_data *header)
{
	console cons{
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

	cons.set_color(0x21, 0x00);
	cons.puts("Popcorn OS ");
	cons.set_color(0x08, 0x00);
	cons.puts(GIT_VERSION " booting...\n");

	return cons;
}

void
kernel_main(popcorn_data *header)
{
	console cons = load_console(header);
	memory_manager::create(
			header->memory_map,
			header->memory_map_length,
			header->memory_map_desc_size);

	interrupts_init();
	interrupts_enable();

	cons.puts("Interrupts initialized.\n");

	device_manager devices(header->acpi_table);

	// int x = 1 / 0;
	// __asm__ __volatile__("int $15");

	cons.puts("boogity!");
	do_the_set_registers(header);
}
