#include <stddef.h>
#include <stdint.h>

#include "j6/signals.h"

#include "kutil/assert.h"
#include "apic.h"
#include "block_device.h"
#include "clock.h"
#include "console.h"
#include "cpu.h"
#include "device_manager.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "io.h"
#include "kernel_args.h"
#include "kernel_memory.h"
#include "log.h"
#include "msr.h"
#include "objects/channel.h"
#include "objects/event.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "scheduler.h"
#include "serial.h"
#include "symbol_table.h"
#include "syscall.h"
#include "tss.h"
#include "vm_space.h"

#ifndef GIT_VERSION
#define GIT_VERSION
#endif

extern "C" {
	void kernel_main(kernel::args::header *header);
	void (*__ctors)(void);
	void (*__ctors_end)(void);
	void long_ap_startup();
	void ap_startup();
	void init_ap_trampoline(void*, uintptr_t, void (*)());
}

extern void __kernel_assert(const char *, unsigned, const char *);

using namespace kernel;

volatile size_t ap_startup_count;

/// Bootstrap the memory managers.
void memory_initialize_pre_ctors(args::header &kargs);
void memory_initialize_post_ctors(args::header &kargs);
process * load_simple_process(args::program &program);

void start_aps(void *kpml4);

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

void
kernel_main(args::header *header)
{
	kutil::assert_set_callback(__kernel_assert);

	init_console();
	logger_init();

	cpu_validate();

	log::debug(logs::boot, "    jsix header is at: %016lx", header);
	log::debug(logs::boot, "     Memory map is at: %016lx", header->mem_map);
	log::debug(logs::boot, "ACPI root table is at: %016lx", header->acpi_table);
	log::debug(logs::boot, "Runtime service is at: %016lx", header->runtime_services);
	log::debug(logs::boot, "    Kernel PML4 is at: %016lx", header->pml4);

	uint64_t cr0, cr4;
	asm ("mov %%cr0, %0" : "=r"(cr0));
	asm ("mov %%cr4, %0" : "=r"(cr4));
	uint64_t efer = rdmsr(msr::ia32_efer);
	log::debug(logs::boot, "Control regs: cr0:%lx cr4:%lx efer:%lx", cr0, cr4, efer);

	bool has_video = false;
	if (header->video.size > 0) {
		has_video = true;
		fb = memory::to_virtual<args::framebuffer>(reinterpret_cast<uintptr_t>(&header->video));

		const args::framebuffer &video = header->video;
		log::debug(logs::boot, "Framebuffer: %dx%d[%d] type %d @ %llx size %llx",
			video.horizontal,
			video.vertical,
			video.scanline,
			video.type,
			video.phys_addr,
			video.size);
		logger_clear_immediate();
	}

	extern TSS &g_bsp_tss;
	extern GDT &g_bsp_gdt;

	TSS *tss = new (&g_bsp_tss) TSS;
	GDT *gdt = new (&g_bsp_gdt) GDT {tss};
	gdt->install();

	IDT *idt = new (&g_idt) IDT;
	idt->install();

	disable_legacy_pic();

	memory_initialize_pre_ctors(*header);
	init_cpu(true);
	run_constructors();
	memory_initialize_post_ctors(*header);

	for (size_t i = 0; i < header->num_modules; ++i) {
		args::module &mod = header->modules[i];
		void *virt = memory::to_virtual<void>(mod.location);

		switch (mod.type) {
		case args::mod_type::symbol_table:
			new symbol_table {virt, mod.size};
			break;

		default:
			break;
		}
	}

	syscall_initialize();

	device_manager &devices = device_manager::get();
	devices.parse_acpi(header->acpi_table);

	devices.init_drivers();
	devices.get_lapic().calibrate_timer();

	start_aps(header->pml4);

	interrupts_enable();

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

	scheduler *sched = new scheduler(devices.get_lapic());

	// Skip program 0, which is the kernel itself
	for (unsigned i = 1; i < header->num_programs; ++i)
		load_simple_process(header->programs[i]);

	if (!has_video)
		sched->create_kernel_task(logger_task, scheduler::max_priority/2, true);

	sched->start();
}

void
start_aps(void *kpml4)
{
	using memory::frame_size;
	using memory::kernel_stack_pages;

	extern size_t ap_startup_code_size;
	extern process &g_kernel_process;
	extern vm_area_guarded &g_kernel_stacks;

	clock &clk = clock::get();
	lapic &apic = device_manager::get().get_lapic();

	ap_startup_count = 1; // BSP processor
	auto &ids = device_manager::get().get_apic_ids();
	log::info(logs::boot, "Starting %d other CPUs", ids.count() - 1);

	// Since we're using address space outside kernel space, make sure
	// the kernel's vm_space is used
	cpu_data &cpu = current_cpu();
	cpu.process = &g_kernel_process;

	// Copy the startup code somwhere the real mode trampoline can run
	uintptr_t addr = 0x8000; // TODO: find a valid address, rewrite addresses
	uint8_t vector = addr >> 12;
	vm_area *vma = new vm_area_fixed(addr, 0x1000, vm_flags::write);
	vm_space::kernel_space().add(addr, vma);
	kutil::memcpy(
		reinterpret_cast<void*>(addr),
		reinterpret_cast<void*>(&ap_startup),
		ap_startup_code_size);

	static constexpr size_t stack_bytes = kernel_stack_pages * frame_size;

	for (uint8_t id : ids) {
		if (id == apic.get_id()) continue;
		log::info(logs::boot, "Starting AP %d", id);

		size_t current_count = ap_startup_count;
		uintptr_t stack_start = g_kernel_stacks.get_section();
		uintptr_t stack_end = stack_start + stack_bytes - 2 * sizeof(void*);
		*reinterpret_cast<uint64_t*>(stack_end) = 0; // pre-fault the page

		init_ap_trampoline(kpml4, stack_end, long_ap_startup);

		apic.send_ipi(lapic::ipi_mode::init, 0, id);
		clk.spinwait(1000);

		apic.send_ipi(lapic::ipi_mode::startup, vector, id);
		for (unsigned i = 0; i < 20; ++i) {
			if (ap_startup_count > current_count) break;
			clk.spinwait(10);
		}

		if (ap_startup_count > current_count)
			continue;

		apic.send_ipi(lapic::ipi_mode::startup, vector, id);
		for (unsigned i = 0; i < 100; ++i) {
			if (ap_startup_count > current_count) break;
			clk.spinwait(10);
		}
	}

	log::info(logs::boot, "%d CPUs running", ap_startup_count);
	vm_space::kernel_space().remove(vma);
}

void
long_ap_startup()
{
	init_cpu(false);
	++ap_startup_count;

	while(1) asm("hlt");
}
