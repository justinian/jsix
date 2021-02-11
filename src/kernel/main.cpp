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
	void long_ap_startup(cpu_data *cpu);
	void ap_startup();
	void init_ap_trampoline(void*, cpu_data *, void (*)(cpu_data *));
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
		fb = &header->video;

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

	extern IDT &g_idt;
	extern TSS &g_bsp_tss;
	extern GDT &g_bsp_gdt;
	extern cpu_data g_bsp_cpu_data;

	IDT *idt = new (&g_idt) IDT;

	cpu_data *cpu = &g_bsp_cpu_data;
	kutil::memset(cpu, 0, sizeof(cpu_data));

	cpu->self = cpu;
	cpu->tss = new (&g_bsp_tss) TSS;
	cpu->gdt = new (&g_bsp_gdt) GDT {cpu->tss};
	cpu_early_init(cpu);

	disable_legacy_pic();

	memory_initialize_pre_ctors(*header);
	run_constructors();
	memory_initialize_post_ctors(*header);

	cpu->tss->create_ist_stacks(idt->used_ist_entries());

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

	syscall_initialize();

	device_manager &devices = device_manager::get();
	devices.parse_acpi(header->acpi_table);

	// Need the local APIC to get the BSP's id
	lapic &apic = device_manager::get().get_lapic();
	cpu->id = apic.get_id();

	cpu_init(cpu, true);

	devices.init_drivers();
	devices.get_lapic().calibrate_timer();

	start_aps(header->pml4);

	idt->add_ist_entries();
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
	cpu_data &bsp = current_cpu();
	bsp.process = &g_kernel_process;

	uint16_t index = bsp.index;

	// Copy the startup code somwhere the real mode trampoline can run
	uintptr_t addr = 0x8000; // TODO: find a valid address, rewrite addresses
	uint8_t vector = addr >> 12;
	vm_area *vma = new vm_area_fixed(addr, 0x1000, vm_flags::write);
	vm_space::kernel_space().add(addr, vma);
	kutil::memcpy(
		reinterpret_cast<void*>(addr),
		reinterpret_cast<void*>(&ap_startup),
		ap_startup_code_size);

	// AP idle stacks need less room than normal stacks, so pack multiple
	// into a normal stack area
	static constexpr size_t idle_stack_bytes = 1024; // 2KiB is generous
	static constexpr size_t full_stack_bytes = kernel_stack_pages * frame_size;
	static constexpr size_t idle_stacks_per = full_stack_bytes / idle_stack_bytes;

	uint8_t ist_entries = IDT::get().used_ist_entries();

	size_t free_stack_count = 0;
	uintptr_t stack_area_start = 0;

	for (uint8_t id : ids) {
		if (id == apic.get_id()) continue;

		// Set up the CPU data structures
		TSS *tss = new TSS;
		GDT *gdt = new GDT {tss};
		cpu_data *cpu = new cpu_data;
		kutil::memset(cpu, 0, sizeof(cpu_data));
		cpu->self = cpu;
		cpu->id = id;
		cpu->index = ++index;
		cpu->gdt = gdt;
		cpu->tss = tss;

		tss->create_ist_stacks(ist_entries);

		// Set up the CPU's idle task stack
		if (free_stack_count == 0) {
			stack_area_start = g_kernel_stacks.get_section();
			free_stack_count = idle_stacks_per;
		}

		uintptr_t stack_end = stack_area_start + free_stack_count-- * idle_stack_bytes;
		stack_end -= 2 * sizeof(void*); // Null frame
		*reinterpret_cast<uint64_t*>(stack_end) = 0; // pre-fault the page
		cpu->rsp0 = stack_end;

		// Set up the trampoline with this CPU's data
		init_ap_trampoline(kpml4, cpu, long_ap_startup);

		// Kick it off!
		size_t current_count = ap_startup_count;
		log::debug(logs::boot, "Starting AP %d: stack %llx", cpu->index, stack_end);
		apic.send_ipi(lapic::ipi_mode::init, 0, id);
		clk.spinwait(1000);

		apic.send_ipi(lapic::ipi_mode::startup, vector, id);
		for (unsigned i = 0; i < 20; ++i) {
			if (ap_startup_count > current_count) break;
			clk.spinwait(20);
		}

		// If the CPU already incremented ap_startup_count, it's done
		if (ap_startup_count > current_count)
			continue;

		// Send the second SIPI (intel recommends this)
		apic.send_ipi(lapic::ipi_mode::startup, vector, id);
		for (unsigned i = 0; i < 100; ++i) {
			if (ap_startup_count > current_count) break;
			clk.spinwait(100);
		}

		log::warn(logs::boot, "No response from AP %d within timeout", id);
	}

	log::info(logs::boot, "%d CPUs running", ap_startup_count);
	vm_space::kernel_space().remove(vma);
}

void
long_ap_startup(cpu_data *cpu)
{
	cpu_init(cpu, false);
	++ap_startup_count;

	while(1) asm("hlt");
}
