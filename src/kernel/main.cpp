#include <stddef.h>
#include <stdint.h>

#include "j6/signals.h"

#include "initrd/initrd.h"
#include "kutil/assert.h"
#include "kutil/vm_space.h"
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
#include "objects/event.h"
#include "objects/handle.h"
#include "page_manager.h"
#include "scheduler.h"
#include "serial.h"
#include "symbol_table.h"
#include "syscall.h"

extern "C" {
	void kernel_main(kernel::args::header *header);
	void (*__ctors)(void);
	void (*__ctors_end)(void);
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

extern void __kernel_assert(const char *, unsigned, const char *);

/// Bootstrap the memory managers.
void memory_initialize_pre_ctors(kernel::args::header *kargs);
void memory_initialize_post_ctors(kernel::args::header *kargs);

using namespace kernel;

/*
class test_observer :
	public kobject::observer
{
public:
	test_observer(const char *name) : m_name(name) {}

	virtual bool on_signals_changed(
			kobject *obj,
			j6_signal_t s,
			j6_signal_t ds,
			j6_status_t result)
	{
		log::info(logs::objs, "  %s: Signals %016lx changed, object %p, result %016lx",
				m_name, ds, obj, result);
		return false;
	}

	const char *m_name;
};
*/

void
init_console()
{
	serial_port *com1 = new (&g_com1) serial_port(COM1);
	console *cons = new (&g_console) console(com1);

	cons->set_color(0x21, 0x00);
	cons->puts("jsix OS ");
	cons->set_color(0x08, 0x00);
	cons->puts(GIT_VERSION " booting...\n");
	logger_init();
}

void
kernel_main(args::header *header)
{
	kutil::assert_set_callback(__kernel_assert);
	init_console();

	gdt_init();
	interrupts_init();

	memory_initialize_pre_ctors(header);
	run_constructors();
	memory_initialize_post_ctors(header);

	cpu_id cpu;
	cpu.validate();

	/*
	if (header->frame_buffer && header->frame_buffer_length) {
		page_manager::get()->map_offset_pointer(
				&header->frame_buffer,
				header->frame_buffer_length);
	}
	*/

	log::debug(logs::boot, "    jsix header is at: %016lx", header);
	log::debug(logs::boot, "     Memory map is at: %016lx", header->mem_map);
	log::debug(logs::boot, "ACPI root table is at: %016lx", header->acpi_table);
	log::debug(logs::boot, "Runtime service is at: %016lx", header->runtime_services);

	// Load the module tagged as initrd
	kutil::vector<initrd::disk> initrds;
	for (unsigned i = 0; i < header->num_modules; ++i) {
		args::module &mod = header->modules[i];
		if (mod.type != args::mod_type::initrd)
			continue;

		initrd::disk &ird = initrds.emplace(mod.location);
		log::info(logs::boot, "initrd loaded with %d files.", ird.files().count());
		for (auto &f : ird.files()) {
			char type = f.executable() ? '*' :
				f.symbols() ? '+' : ' ';
			log::info(logs::boot, "  %c%s (%d bytes).", type, f.name(), f.size());
		}
	}

	/*
	   page_manager::get()->dump_pml4(nullptr, 0);
	   page_manager::get()->dump_blocks(true);
	*/

	device_manager &devices = device_manager::get();
	devices.parse_acpi(header->acpi_table);

	interrupts_enable();

	/*
	auto r = cpu.get(0x15);
	log::info(logs::boot, "CPU Crystal: %dHz", r.ecx);

	uintptr_t cr4 = 0;
	__asm__ __volatile__ ( "mov %%cr4, %0" : "=r" (cr4) );
	log::info(logs::boot, "cr4: %016x", cr4);
	*/

	devices.init_drivers();

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

	devices.get_lapic()->calibrate_timer();
	devices.init_drivers();

	syscall_enable();
	scheduler *sched = new scheduler(devices.get_lapic());

	for (auto &ird : initrds) {
		for (auto &f : ird.files()) {
			if (f.executable()) {
				sched->load_process(f.name(), f.data(), f.size());
			} else if (f.symbols()) {
				new symbol_table {f.data(), f.size()};
			}
		}
	}

	/*
	log::info(logs::objs, "Testing object system:");
	test_observer obs1("event");
	test_observer obs2("no handles");
	{
		event e;

		e.register_signal_observer(&obs1, j6_signal_user0);
		e.register_signal_observer(&obs2, j6_signal_no_handles);

		e.assert_signal(j6_signal_user0);

		handle h(1, 0, &e);
	}
	*/

	sched->create_kernel_task(logger_task, scheduler::max_priority-1, true);
	sched->start();
}
