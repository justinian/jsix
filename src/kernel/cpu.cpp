#include <new>
#include <stdint.h>
#include <string.h>
#include <util/bitset.h>

#include "assert.h"
#include "cpu.h"
#include "cpu/cpu_id.h"
#include "device_manager.h"
#include "gdt.h"
#include "idt.h"
#include "logger.h"
#include "msr.h"
#include "objects/thread.h"
#include "syscall.h"
#include "tss.h"

unsigned g_num_cpus = 1;

panic_data g_panic_data;
panic_data *g_panic_data_p = &g_panic_data;

cpu_data g_bsp_cpu_data;
cpu_data **g_cpu_data = nullptr;


// Validate the required CPU features are present. Really, the bootloader already
// validated the required features, but still iterate the options and log about them.
void
cpu_validate()
{
    cpu::cpu_id cpu;

    char brand_name[50];
    cpu.brand_name(brand_name);

    cpu::cpu_id::features features = cpu.validate();

    log::info(logs::boot, "CPU: %s", brand_name);
    log::info(logs::boot, "    Vendor is %s", cpu.vendor_id());

    log::spam(logs::boot, "    Higest basic CPUID: 0x%02x", cpu.highest_basic());
    log::spam(logs::boot, "    Higest ext CPUID:   0x%02x", cpu.highest_ext() & ~cpu::cpu_id::cpuid_extended);

#define CPU_FEATURE_OPT(name, ...) \
    log::verbose(logs::boot, "    Supports %9s: %s", #name, features[cpu::feature::name] ? "yes" : "no");

#define CPU_FEATURE_REQ(name, feat_leaf, feat_sub, regname, bit) \
    log::verbose(logs::boot, "    Supports %9s: %s", #name, features[cpu::feature::name] ? "yes" : "no"); \
    kassert(features[cpu::feature::name], "Missing required CPU feature " #name );

#include "cpu/features.inc"
#undef CPU_FEATURE_OPT
#undef CPU_FEATURE_REQ
}


// Do early (before cpu_init) initialization work. Only needs to be called manually for
// the BSP, otherwise cpu_init will call it.
static void
cpu_early_init(cpu_data *cpu)
{
    cpu->idt->install();
    cpu->gdt->install();

    util::bitset64 cr4_val = 0;
    asm ("mov %%cr4, %0" : "=r"(cr4_val));
    cr4_val
        .set(cr4::OSXFSR)
        .set(cr4::OSXMMEXCPT)
        .set(cr4::PCIDE)
        .set(cr4::OSXSAVE);
    asm volatile ( "mov %0, %%cr4" :: "r" (cr4_val) );

    // Enable SYSCALL and NX bit
    util::bitset64 efer_val = rdmsr(msr::ia32_efer);
    efer_val
        .set(efer::SCE)
        .set(efer::NXE);
    wrmsr(msr::ia32_efer, efer_val);

    // Install the GS base pointint to the cpu_data
    wrmsr(msr::ia32_gs_base, reinterpret_cast<uintptr_t>(cpu));
}

cpu_data *
bsp_early_init()
{
    cpu_validate();
    memset(&g_panic_data, 0, sizeof(g_panic_data));

    extern IDT &g_bsp_idt;
    extern TSS &g_bsp_tss;
    extern GDT &g_bsp_gdt;
    extern uintptr_t idle_stack_end;

    cpu_data *cpu = &g_bsp_cpu_data;
    memset(cpu, 0, sizeof(cpu_data));

    cpu->self = cpu;
    cpu->idt = new (&g_bsp_idt) IDT;
    cpu->tss = new (&g_bsp_tss) TSS;
    cpu->gdt = new (&g_bsp_gdt) GDT {cpu->tss};
    cpu->rsp0 = reinterpret_cast<uintptr_t>(&idle_stack_end);
    cpu_early_init(cpu);

    return cpu;
}

void
bsp_late_init()
{
    // BSP didn't set up IST stacks yet
    extern TSS &g_bsp_tss;
    uint8_t ist_entries = IDT::used_ist_entries();
    g_bsp_tss.create_ist_stacks(ist_entries);

    uint64_t cr0v, cr4v;
    asm ("mov %%cr0, %0" : "=r"(cr0v));
    asm ("mov %%cr4, %0" : "=r"(cr4v));

    uint64_t efer = rdmsr(msr::ia32_efer);
    log::spam(logs::boot, "Control regs: cr0:%lx cr4:%lx efer:%lx", cr0v, cr4v, efer);
}

cpu_data *
cpu_create(uint16_t id, uint16_t index)
{
    // Set up the CPU data structures
    IDT *idt = new IDT;
    TSS *tss = new TSS;
    GDT *gdt = new GDT {tss};
    cpu_data *cpu = new cpu_data;
    memset(cpu, 0, sizeof(cpu_data));
    g_cpu_data[index] = cpu;

    cpu->self = cpu;
    cpu->id = id;
    cpu->index = index;
    cpu->idt = idt;
    cpu->tss = tss;
    cpu->gdt = gdt;

    uint8_t ist_entries = IDT::used_ist_entries();
    tss->create_ist_stacks(ist_entries);

    return cpu;
}

void
cpu_init(cpu_data *cpu, bool bsp)
{
    if (!bsp) {
        // The BSP already called cpu_early_init
        cpu_early_init(cpu);
    }

    // Set the initial process as the kernel "process"
    extern obj::process &g_kernel_process;
    cpu->process = &g_kernel_process;

    obj::thread *idle = obj::thread::create_idle_thread(
            g_kernel_process,
            cpu->rsp0);

    cpu->thread = idle;
    cpu->tcb = idle->tcb();

    // Set up the syscall MSRs
    syscall_enable();

    // Set up the page attributes table
    uint64_t pat = rdmsr(msr::ia32_pat);
    pat = (pat & 0x00ffffffffffffffull) | (0x01ull << 56); // set PAT 7 to WC
    wrmsr(msr::ia32_pat, pat);

    cpu->idt->add_ist_entries();

    uintptr_t apic_base =
        device_manager::get().get_lapic_base();

    lapic *apic = new lapic(apic_base);
    cpu->apic = apic;
    apic->enable();

    if (bsp) {
        // BSP never got an id, set that up now
        cpu->id = apic->get_id();
        apic->calibrate_timer();
    }
}
