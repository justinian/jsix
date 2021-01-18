#include <stdint.h>
#include "kutil/assert.h"
#include "kutil/memory.h"
#include "cpu.h"
#include "cpu/cpu.h"
#include "log.h"

cpu_data bsp_cpu_data;

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
