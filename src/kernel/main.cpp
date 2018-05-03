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
}

/*
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

	return cons;
}
*/

void
kernel_main(popcorn_data *header)
{
	serial_port *com1 = new (&g_com1) serial_port(COM1);
	console *cons = new (&g_console) console(com1);

	cons->set_color(0x21, 0x00);
	cons->puts("Popcorn OS ");
	cons->set_color(0x08, 0x00);
	cons->puts(GIT_VERSION " booting...\n");

	log::init(cons);
	log::enable(logs::interrupt, log::level::debug);

	cpu_id cpu;

	log::info(logs::boot, "CPU Vendor: %s", cpu.vendor_id());
	log::info(logs::boot, "CPU Family %x Model %x Stepping %x",
			cpu.family(), cpu.model(), cpu.stepping());

	cpu_id::regs cpu_info = cpu.get(1);
	log::info(logs::boot, "LAPIC Supported: %s",
			cpu_info.edx_bit(9) ? "yes" : "no");
	log::info(logs::boot, "x2APIC Supported: %s",
			cpu_info.ecx_bit(21) ? "yes" : "no");

	page_manager *pager = new (&g_page_manager) page_manager;

	memory_initialize_managers(
			header->memory_map,
			header->memory_map_length,
			header->memory_map_desc_size);

	pager->map_offset_pointer(&header->frame_buffer, header->frame_buffer_length);

	interrupts_init();

	device_manager devices(header->acpi_table);
	interrupts_enable();

	// int x = 1 / 0;
	// __asm__ __volatile__("int $15");

	g_console.puts("boogity!");
	do_the_set_registers(header);
}
