#include <stdint.h>
#include "kutil/assert.h"
#include "kutil/memory.h"
#include "apic.h"
#include "cpu.h"
#include "cpu/cpu_id.h"
#include "device_manager.h"
#include "gdt.h"
#include "idt.h"
#include "kernel_memory.h"
#include "log.h"
#include "msr.h"
#include "objects/vm_area.h"
#include "tss.h"

cpu_data g_bsp_cpu_data;

void
cpu_validate()
{
	cpu::cpu_id cpu;

	log::info(logs::boot, "CPU: %s", cpu.brand_name());
	log::debug(logs::boot, "    Vendor is %s", cpu.vendor_id());

	log::debug(logs::boot, "    Higest basic CPUID: 0x%02x", cpu.highest_basic());
	log::debug(logs::boot, "    Higest ext CPUID:   0x%02x", cpu.highest_ext() & ~cpu::cpu_id::cpuid_extended);

#define CPU_FEATURE_OPT(name, ...) \
	log::debug(logs::boot, "    Supports %9s: %s", #name, cpu.has_feature(cpu::feature::name) ? "yes" : "no");

#define CPU_FEATURE_REQ(name, feat_leaf, feat_sub, regname, bit) \
	CPU_FEATURE_OPT(name, feat_leaf, feat_sub, regname, bit); \
	kassert(cpu.has_feature(cpu::feature::name), "Missing required CPU feature " #name );

#include "cpu/features.inc"
#undef CPU_FEATURE_OPT
#undef CPU_FEATURE_REQ
}

void
init_cpu(bool bsp)
{
	extern TSS &g_bsp_tss;
	extern GDT &g_bsp_gdt;
	extern vm_area_guarded &g_kernel_stacks;

	uint8_t id = 0;

	TSS *tss = nullptr;
	GDT *gdt = nullptr;
	cpu_data *cpu = nullptr;

	if (bsp) {
		gdt = &g_bsp_gdt;
		tss = &g_bsp_tss;
		cpu = &g_bsp_cpu_data;
	} else {
		g_idt.install();

		tss = new TSS;
		gdt = new GDT {tss};
		cpu = new cpu_data;

		gdt->install();

		lapic &apic = device_manager::get().get_lapic();
		id = apic.get_id();
	}

	kutil::memset(cpu, 0, sizeof(cpu_data));

	cpu->self = cpu;
	cpu->id = id;
	cpu->gdt = gdt;
	cpu->tss = tss;

	// Install the GS base pointint to the cpu_data
	wrmsr(msr::ia32_gs_base, reinterpret_cast<uintptr_t>(cpu));

	using memory::frame_size;
	using memory::kernel_stack_pages;
	constexpr size_t stack_size = kernel_stack_pages * frame_size;

	uint8_t ist_entries = g_idt.used_ist_entries();

	// Set up the IST stacks
	for (unsigned ist = 1; ist < 8; ++ist) {
		if (!(ist_entries & (1 << ist)))
			continue;

		// Two zero entries at the top for the null frame
		uintptr_t stack_bottom = g_kernel_stacks.get_section();
		uintptr_t stack_top = stack_bottom + stack_size - 2 * sizeof(uintptr_t);

		// Pre-realize these stacks, they're no good if they page fault
		*reinterpret_cast<uint64_t*>(stack_top) = 0;

		tss->ist_stack(ist) = stack_top;
	}

	// Set up the page attributes table
	uint64_t pat = rdmsr(msr::ia32_pat);
	pat = (pat & 0x00ffffffffffffffull) | (0x01ull << 56); // set PAT 7 to WC
	wrmsr(msr::ia32_pat, pat);
}
