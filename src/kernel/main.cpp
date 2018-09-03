#include <stddef.h>
#include <stdint.h>

#include "kutil/assert.h"
#include "kutil/memory.h"
#include "block_device.h"
#include "console.h"
#include "cpu.h"
#include "device_manager.h"
#include "font.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_data.h"
#include "log.h"
#include "memory.h"
#include "page_manager.h"
#include "scheduler.h"
#include "screen.h"
#include "serial.h"

extern "C" {
	void kernel_main(popcorn_data *header);
	void syscall_enable();

	void *__bss_start, *__bss_end;
}

extern void __kernel_assert(const char *, unsigned, const char *);

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
	log::enable(logs::device, log::level::debug);
	log::enable(logs::driver, log::level::debug);
	log::enable(logs::memory, log::level::info);
	log::enable(logs::fs, log::level::debug);
	log::enable(logs::task, log::level::debug);
}

void do_error_3() { volatile int x = 1; volatile int y = 0; volatile int z = x / y; }
void do_error_2() { do_error_3(); }
void do_error_1() { do_error_2(); }

void
kernel_main(popcorn_data *header)
{
	kutil::assert_set_callback(__kernel_assert);

	gdt_init();
	interrupts_init();

	page_manager *pager = new (&g_page_manager) page_manager;

	memory_initialize(
			header->memory_map,
			header->memory_map_length,
			header->memory_map_desc_size);

	pager->map_offset_pointer(
			&header->frame_buffer,
			header->frame_buffer_length);

	init_console(header);

	log::debug(logs::boot, " Popcorn header is at: %016lx", header);
	log::debug(logs::boot, "    Framebuffer is at: %016lx", header->frame_buffer);
	log::debug(logs::boot, "      Font data is at: %016lx", header->font);
	log::debug(logs::boot, "    Kernel data is at: %016lx", header->data);
	log::debug(logs::boot, "     Memory map is at: %016lx", header->memory_map);
	log::debug(logs::boot, "ACPI root table is at: %016lx", header->acpi_table);
	log::debug(logs::boot, "Runtime service is at: %016lx", header->runtime);

	/*
	pager->dump_pml4(nullptr, 0);
	pager->dump_blocks(true);
	*/

	device_manager *devices =
		new (&device_manager::get()) device_manager(header->acpi_table);

	interrupts_enable();

	/*
	cpu_id cpu;
	log::info(logs::boot, "CPU Vendor: %s", cpu.vendor_id());
	log::info(logs::boot, "CPU Family %x Model %x Stepping %x",
			cpu.family(), cpu.model(), cpu.stepping());

	auto r = cpu.get(0x15);
	log::info(logs::boot, "CPU Crystal: %dHz", r.ecx);

	addr_t cr4 = 0;
	__asm__ __volatile__ ( "mov %%cr4, %0" : "=r" (cr4) );
	log::info(logs::boot, "cr4: %016x", cr4);
	*/

	devices->init_drivers();

	/*
	block_device *disk = devices->get_block_device(0);
	if (disk) {
		for (int i=0; i<1; ++i) {
			uint8_t buf[512];
			kutil::memset(buf, 0, 512);

			kassert(disk->read(0x200, sizeof(buf), buf),
					"Disk read returned 0");

			console *cons = console::get();
			uint8_t *p = &buf[0];
			for (int i = 0; i < 8; ++i) {
				for (int j = 0; j < 16; ++j) {
					cons->printf(" %02x", *p++);
				}
				cons->putc('\n');
			}
		}
	} else {
		log::warn(logs::boot, "No block devices present.");
	}
	*/

	// do_error_1();
	// __asm__ __volatile__("int $15");

	// pager->dump_pml4();

	syscall_enable();
	scheduler *sched = new (&scheduler::get()) scheduler(devices->get_lapic());
	sched->start();

	g_console.puts("boogity!\n");
}
