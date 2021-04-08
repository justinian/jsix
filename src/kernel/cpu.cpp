#include <stdint.h>
#include "kutil/assert.h"
#include "kutil/memory.h"
#include "cpu.h"
#include "cpu/cpu_id.h"
#include "device_manager.h"
#include "gdt.h"
#include "idt.h"
#include "kernel_memory.h"
#include "log.h"
#include "msr.h"
#include "objects/vm_area.h"
#include "syscall.h"
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
cpu_early_init(cpu_data *cpu)
{
	cpu->idt->install();
	cpu->gdt->install();

	// Install the GS base pointint to the cpu_data
	wrmsr(msr::ia32_gs_base, reinterpret_cast<uintptr_t>(cpu));

	// Set the initial process as the kernel "process"
	extern process &g_kernel_process;
	cpu->process = &g_kernel_process;
}

void
cpu_init(cpu_data *cpu, bool bsp)
{
	if (!bsp) {
		// The BSP already called cpu_early_init
		cpu_early_init(cpu);
	}

	// Set up the syscall MSRs
	syscall_enable();

	// Set up the page attributes table
	uint64_t pat = rdmsr(msr::ia32_pat);
	pat = (pat & 0x00ffffffffffffffull) | (0x01ull << 56); // set PAT 7 to WC
	wrmsr(msr::ia32_pat, pat);

	cpu->idt->add_ist_entries();
}
