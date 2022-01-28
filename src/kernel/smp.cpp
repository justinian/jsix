#include <stdint.h>

#include "apic.h"
#include "clock.h"
#include "device_manager.h"
#include "logger.h"
#include "memory.h"
#include "objects/vm_area.h"
#include "scheduler.h"
#include "smp.h"
#include "vm_space.h"

extern "C" {
    void long_ap_startup(cpu_data *cpu);
    void ap_startup();
    void ap_idle();
    void init_ap_trampoline(void*, cpu_data *, void (*)());
}

extern size_t ap_startup_code_size;
extern obj::process &g_kernel_process;
extern obj::vm_area_guarded &g_kernel_stacks;

namespace smp {

volatile size_t ap_startup_count;
volatile bool scheduler_ready = false;


unsigned
start(cpu_data &bsp, void *kpml4)
{
    using mem::frame_size;
    using mem::kernel_stack_pages;
    using obj::vm_flags;


    ap_startup_count = 1; // Count the BSP

    clock &clk = clock::get();

    const auto &ids = device_manager::get().get_apic_ids();

    log::info(logs::boot, "Starting %d other CPUs", ids.count() - 1);

    // Since we're using address space outside kernel space, make sure
    // the kernel's vm_space is used
    bsp.process = &g_kernel_process;

    uint16_t index = bsp.index;

    // Copy the startup code somwhere the real mode trampoline can run
    uintptr_t addr = 0x8000; // TODO: find a valid address, rewrite addresses
    uint8_t vector = addr >> 12;
    obj::vm_area *vma = new obj::vm_area_fixed(addr, 0x1000, vm_flags::write);
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

    size_t free_stack_count = 0;
    uintptr_t stack_area_start = 0;

    lapic &apic = *bsp.apic;
    lapic::ipi mode = lapic::ipi::init | lapic::ipi::level | lapic::ipi::assert;
    apic.send_ipi_broadcast(mode, false, 0);

    for (uint8_t id : ids) {
        if (id == bsp.id) continue;

        cpu_data *cpu = cpu_create(id, ++index);

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
ready()
{
    scheduler_ready = true;
}

} // namespace smp

void
long_ap_startup(cpu_data *cpu)
{
    __atomic_add_fetch(&smp::ap_startup_count, 1, __ATOMIC_SEQ_CST);

    cpu_init(cpu, false);

    while (!smp::scheduler_ready) asm ("pause");
    scheduler::get().start();
}
