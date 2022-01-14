#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <bootproto/kernel.h>
#include <j6/signals.h>
#include <util/vector.h>

#include "apic.h"
#include "assert.h"
#include "block_device.h"
#include "clock.h"
#include "console.h"
#include "cpu.h"
#include "device_manager.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"
#include "memory.h"
#include "msr.h"
#include "objects/channel.h"
#include "objects/event.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "scheduler.h"
#include "serial.h"
#include "syscall.h"
#include "sysconf.h"
#include "tss.h"
#include "vm_space.h"


#ifndef GIT_VERSION
#define GIT_VERSION
#endif

extern "C" {
    void kernel_main(bootproto::args *args);
    void (*__ctors)(void);
    void (*__ctors_end)(void);
    void long_ap_startup(cpu_data *cpu);
    void ap_startup();
    void ap_idle();
    void init_ap_trampoline(void*, cpu_data *, void (*)());
}

volatile size_t ap_startup_count;
static bool scheduler_ready = false;

/// Bootstrap the memory managers.
void memory_initialize_pre_ctors(bootproto::args &kargs);
void memory_initialize_post_ctors(bootproto::args &kargs);
void load_init_server(bootproto::program &program, uintptr_t modules_address);

unsigned start_aps(lapic &apic, const util::vector<uint8_t> &ids, void *kpml4);

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
        if (ctor) ctor();
    }
}

void
kernel_main(bootproto::args *args)
{
    if (args->panic) {
        IDT::set_nmi_handler(args->panic->entrypoint);
        panic::symbol_table = util::offset_pointer(args->symbol_table, mem::linear_offset);
    }

    init_console();
    logger_init();

    cpu_validate();

    extern IDT &g_bsp_idt;
    extern TSS &g_bsp_tss;
    extern GDT &g_bsp_gdt;
    extern cpu_data g_bsp_cpu_data;
    extern uintptr_t idle_stack_end;

    cpu_data *cpu = &g_bsp_cpu_data;
    memset(cpu, 0, sizeof(cpu_data));

    cpu->self = cpu;
    cpu->idt = new (&g_bsp_idt) IDT;
    cpu->tss = new (&g_bsp_tss) TSS;
    cpu->gdt = new (&g_bsp_gdt) GDT {cpu->tss};
    cpu->rsp0 = idle_stack_end;
    cpu_early_init(cpu);

    kassert(args->magic == bootproto::args_magic,
            "Bad kernel args magic number");

    log::debug(logs::boot, "jsix init args are at: %016lx", args);
    log::debug(logs::boot, "     Memory map is at: %016lx", args->mem_map);
    log::debug(logs::boot, "ACPI root table is at: %016lx", args->acpi_table);
    log::debug(logs::boot, "Runtime service is at: %016lx", args->runtime_services);
    log::debug(logs::boot, "    Kernel PML4 is at: %016lx", args->pml4);

    uint64_t cr0, cr4;
    asm ("mov %%cr0, %0" : "=r"(cr0));
    asm ("mov %%cr4, %0" : "=r"(cr4));
    uint64_t efer = rdmsr(msr::ia32_efer);
    log::debug(logs::boot, "Control regs: cr0:%lx cr4:%lx efer:%lx", cr0, cr4, efer);

    disable_legacy_pic();

    memory_initialize_pre_ctors(*args);
    run_constructors();
    memory_initialize_post_ctors(*args);

    cpu->tss->create_ist_stacks(cpu->idt->used_ist_entries());

    syscall_initialize();

    device_manager &devices = device_manager::get();
    devices.parse_acpi(args->acpi_table);

    // Need the local APIC to get the BSP's id
    uintptr_t apic_base = devices.get_lapic_base();

    lapic *apic = new lapic(apic_base);
    apic->enable();

    cpu->id = apic->get_id();
    cpu->apic = apic;

    cpu_init(cpu, true);

    devices.init_drivers();
    apic->calibrate_timer();

    const auto &apic_ids = devices.get_apic_ids();
    g_num_cpus = start_aps(*apic, apic_ids, args->pml4);

    sysconf_create();
    interrupts_enable();
    g_com1.handle_interrupt();

    /*
    block_device *disk = devices->get_block_device(0);
    if (disk) {
        for (int i=0; i<1; ++i) {
            uint8_t buf[512];
            memset(buf, 0, 512);

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

    scheduler *sched = new scheduler {g_num_cpus};
    scheduler_ready = true;

    // Load the init server
    load_init_server(*args->init, args->modules);

    sched->create_kernel_task(logger_task, scheduler::max_priority/4, true);
    sched->start();
}

unsigned
start_aps(lapic &apic, const util::vector<uint8_t> &ids, void *kpml4)
{
    using mem::frame_size;
    using mem::kernel_stack_pages;

    extern size_t ap_startup_code_size;
    extern process &g_kernel_process;
    extern vm_area_guarded &g_kernel_stacks;

    clock &clk = clock::get();

    ap_startup_count = 1; // BSP processor
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
    memcpy(
        reinterpret_cast<void*>(addr),
        reinterpret_cast<void*>(&ap_startup),
        ap_startup_code_size);

    // AP idle stacks need less room than normal stacks, so pack multiple
    // into a normal stack area
    static constexpr size_t idle_stack_bytes = 2048; // 2KiB is generous
    static constexpr size_t full_stack_bytes = kernel_stack_pages * frame_size;
    static constexpr size_t idle_stacks_per = full_stack_bytes / idle_stack_bytes;

    uint8_t ist_entries = IDT::current().used_ist_entries();

    size_t free_stack_count = 0;
    uintptr_t stack_area_start = 0;

    lapic::ipi mode = lapic::ipi::init | lapic::ipi::level | lapic::ipi::assert;
    apic.send_ipi_broadcast(mode, false, 0);

    for (uint8_t id : ids) {
        if (id == bsp.id) continue;

        // Set up the CPU data structures
        IDT *idt = new IDT;
        TSS *tss = new TSS;
        GDT *gdt = new GDT {tss};
        cpu_data *cpu = new cpu_data;
        memset(cpu, 0, sizeof(cpu_data));

        cpu->self = cpu;
        cpu->id = id;
        cpu->index = ++index;
        cpu->idt = idt;
        cpu->tss = tss;
        cpu->gdt = gdt;

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
        init_ap_trampoline(kpml4, cpu, ap_idle);

        // Kick it off!
        size_t current_count = ap_startup_count;
        log::debug(logs::boot, "Starting AP %d: stack %llx", cpu->index, stack_end);

        lapic::ipi startup = lapic::ipi::startup | lapic::ipi::assert;

        apic.send_ipi(startup, vector, id);
        for (unsigned i = 0; i < 20; ++i) {
            if (ap_startup_count > current_count) break;
            clk.spinwait(20);
        }

        // If the CPU already incremented ap_startup_count, it's done
        if (ap_startup_count > current_count)
            continue;

        // Send the second SIPI (intel recommends this)
        apic.send_ipi(startup, vector, id);
        for (unsigned i = 0; i < 100; ++i) {
            if (ap_startup_count > current_count) break;
            clk.spinwait(100);
        }

        log::warn(logs::boot, "No response from AP %d within timeout", id);
    }

    log::info(logs::boot, "%d CPUs running", ap_startup_count);
    vm_space::kernel_space().remove(vma);
    return ap_startup_count;
}

void
long_ap_startup(cpu_data *cpu)
{
    cpu_init(cpu, false);
    ++ap_startup_count;
    while (!scheduler_ready) asm ("pause");

    uintptr_t apic_base =
        device_manager::get().get_lapic_base();
    cpu->apic = new lapic(apic_base);
    cpu->apic->enable();

    scheduler::get().start();
}
