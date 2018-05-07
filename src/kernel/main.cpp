#include <stddef.h>
#include <stdint.h>

#include "kutil/memory.h"
#include "console.h"
#include "cpu.h"
#include "device_manager.h"
#include "font.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_data.h"
#include "log.h"
#include "memory.h"
#include "memory_pages.h"
#include "screen.h"
#include "serial.h"

extern "C" {
	void do_the_set_registers(popcorn_data *header);
	void kernel_main(popcorn_data *header);

	void *__bss_start, *__bss_end;
}

void
init_console(const popcorn_data *header)
{
	serial_port *com1 = new (&g_com1) serial_port(COM1);
	console *cons = new (&g_console) console(com1);

	if (header->frame_buffer) {
		screen *s = new screen(
				header->frame_buffer,
				header->hres,
				header->vres,
				header->rmask,
				header->gmask,
				header->bmask);
		font *f = new font(header->font);
		cons->init_screen(s, f);
	}

	cons->set_color(0x21, 0x00);
	cons->puts("Popcorn OS ");
	cons->set_color(0x08, 0x00);
	cons->puts(GIT_VERSION " booting...\n");

	log::init(cons);
	log::enable(logs::apic, log::level::info);
	log::enable(logs::devices, log::level::info);
	log::enable(logs::memory, log::level::debug);
}

void do_error_3() { int x = 1 / 0; }
void do_error_2() { do_error_3(); }
void do_error_1() { do_error_2(); }

void
kernel_main(popcorn_data *header)
{
	page_manager *pager = new (&g_page_manager) page_manager;

	memory_initialize_managers(
			header->memory_map,
			header->memory_map_length,
			header->memory_map_desc_size);

	pager->map_offset_pointer(
			&header->frame_buffer,
			header->frame_buffer_length);

	init_console(header);
	// pager->dump_blocks();

	interrupts_init();
	device_manager devices(header->acpi_table);
	interrupts_enable();

	cpu_id cpu;
	log::info(logs::boot, "CPU Vendor: %s", cpu.vendor_id());
	log::info(logs::boot, "CPU Family %x Model %x Stepping %x",
			cpu.family(), cpu.model(), cpu.stepping());

	// do_error_1();
	// __asm__ __volatile__("int $15");

	g_console.puts("boogity!");
	do_the_set_registers(header);
}
