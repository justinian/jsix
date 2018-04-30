#include <stddef.h>
#include <stdint.h>

#include "kutil/memory.h"
#include "console.h"
#include "cpu.h"
#include "device_manager.h"
#include "font.h"
#include "interrupts.h"
#include "kernel_data.h"
#include "log.h"
#include "memory.h"
#include "memory_pages.h"
#include "screen.h"

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
	console *cons = new (&g_console) console();
	cons->set_color(0x21, 0x00);
	cons->puts("Popcorn OS ");
	cons->set_color(0x08, 0x00);
	cons->puts(GIT_VERSION " booting...\n");

	log::init(cons);

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
	pager->mark_offset_pointer(&header->frame_buffer, header->frame_buffer_length);

	memory_initialize_managers(
			header->memory_map,
			header->memory_map_length,
			header->memory_map_desc_size);

	interrupts_init();
	interrupts_enable();

	device_manager devices(header->acpi_table);

	// int x = 1 / 0;
	// __asm__ __volatile__("int $15");

	g_console.puts("boogity!");
	do_the_set_registers(header);
}
