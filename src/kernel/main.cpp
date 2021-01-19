#include <stddef.h>
#include <stdint.h>

#include "j6/signals.h"

#include "kutil/assert.h"
#include "apic.h"
#include "block_device.h"
#include "console.h"
#include "cpu.h"
#include "device_manager.h"
#include "gdt.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_args.h"
#include "kernel_memory.h"
#include "log.h"
#include "objects/channel.h"
#include "objects/event.h"
#include "objects/thread.h"
#include "scheduler.h"
#include "serial.h"
#include "symbol_table.h"
#include "syscall.h"

#ifndef GIT_VERSION
#define GIT_VERSION
#endif

extern "C" {
	void kernel_main(kernel::args::header *header);
	void (*__ctors)(void);
	void (*__ctors_end)(void);
}

extern void __kernel_assert(const char *, unsigned, const char *);

/// Bootstrap the memory managers.
void setup_pat();
void memory_initialize_pre_ctors(kernel::args::header *kargs);
void memory_initialize_post_ctors(kernel::args::header *kargs);

using namespace kernel;

/// TODO: not this. this is awful.
args::framebuffer *fb = nullptr;

void
init_console()
{
	serial_port *com1 = new (&g_com1) serial_port(COM1);
	console *cons = new (&g_console) console(com1);

	cons->set_color(0x21, 0x00);
	cons->puts("jsix OS ");
	cons->set_color(0x08, 0x00);
	cons->puts(GIT_VERSION " booting...\n");
}

void
run_constructors()
{
	void (**p)(void) = &__ctors;
	while (p < &__ctors_end) {
		void (*ctor)(void) = *p++;
		ctor();
	}
}

channel *std_out = nullptr;

void
stdout_task()
{
	uint8_t buffer[257];
	auto *ent = reinterpret_cast<log::logger::entry *>(buffer);
	auto *cons = console::get();

	log::info(logs::task, "Starting kernel stdout task");

	scheduler &s = scheduler::get();
	thread *th = thread::from_tcb(s.current());

	while (true) {
		j6_signal_t current = std_out->signals();
		if (!(current & j6_signal_channel_can_recv)) {
			th->wait_on_signals(std_out, j6_signal_channel_can_recv);
			s.schedule();
		}

		size_t n = 256;
		j6_status_t status = std_out->dequeue(&n, buffer);
		if (status != j6_status_ok) {
			log::warn(logs::task, "Kernel stdout error: %x", status);
			return;
		}

		buffer[n] = 0;
		const char *s = reinterpret_cast<const char *>(buffer);

		while (n) {
			size_t r = cons->puts(s);
			n -= r + 1;
			s += r + 1;
		}
	}
}


void
kernel_main(args::header *header)
{
	kutil::assert_set_callback(__kernel_assert);

	init_console();
	logger_init();

	setup_pat();

	bool has_video = false;
	if (header->video.size > 0) {
		has_video = true;
		fb = memory::to_virtual<args::framebuffer>(reinterpret_cast<uintptr_t>(&header->video));

		const args::framebuffer &video = header->video;
		log::debug(logs::boot, "Framebuffer: %dx%d[%d] type %s @ %016llx",
			video.horizontal, video.vertical, video.scanline, video.type, video.phys_addr);
		logger_clear_immediate();
	}

	gdt_init();
	interrupts_init();

	memory_initialize_pre_ctors(header);
	run_constructors();
	memory_initialize_post_ctors(header);

	cpu_validate();

	for (size_t i = 0; i < header->num_modules; ++i) {
		args::module &mod = header->modules[i];
		switch (mod.type) {
		case args::mod_type::symbol_table:
			new symbol_table {mod.location, mod.size};
			break;

		default:
			break;
		}
	}

	log::debug(logs::boot, "    jsix header is at: %016lx", header);
	log::debug(logs::boot, "     Memory map is at: %016lx", header->mem_map);
	log::debug(logs::boot, "ACPI root table is at: %016lx", header->acpi_table);
	log::debug(logs::boot, "Runtime service is at: %016lx", header->runtime_services);

	device_manager &devices = device_manager::get();
	devices.parse_acpi(header->acpi_table);

	interrupts_enable();
	devices.init_drivers();
	devices.get_lapic()->calibrate_timer();

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

	syscall_enable();
	scheduler *sched = new scheduler(devices.get_lapic());

	std_out = new channel;

	// Skip program 0, which is the kernel itself
	for (size_t i = 1; i < header->num_programs; ++i) {
		args::program &prog = header->programs[i];
		thread *th = sched->load_process(prog.phys_addr, prog.virt_addr, prog.size, prog.entrypoint); 
		if (i == 2) {
			//th->set_state(thread::state::constant);
		}
	}

	if (!has_video)
		sched->create_kernel_task(logger_task, scheduler::max_priority/2, true);
	sched->create_kernel_task(stdout_task, scheduler::max_priority-1, true);

	const char stdout_message[] = "Hello on the fake stdout channel\n";
	size_t message_size = sizeof(stdout_message);
	std_out->enqueue(&message_size, reinterpret_cast<const void*>(stdout_message));

	sched->start();
}
