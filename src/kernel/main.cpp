#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <bootproto/kernel.h>
#include <j6/signals.h>
#include <util/vector.h>

#include "assert.h"
#include "cpu.h"
#include "device_manager.h"
#include "interrupts.h"
#include "logger.h"
#include "memory.h"
#include "objects/process.h"
#include "objects/system.h"
#include "objects/thread.h"
#include "objects/vm_area.h"
#include "scheduler.h"
#include "smp.h"
#include "sysconf.h"

extern "C" {
    void kernel_main(bootproto::args *args);
}

/// Bootstrap the memory managers.
void load_init_server(bootproto::program &program, uintptr_t modules_address);


void
kernel_main(bootproto::args *args)
{
    if (args->panic) {
        const void *syms = util::offset_pointer(args->symbol_table, mem::linear_offset);
        panic::install(args->panic->entrypoint, syms);
    }

    logger_init();

    cpu_data *cpu = bsp_early_init();

    kassert(args->magic == bootproto::args_magic,
            "Bad kernel args magic number");

    log::debug(logs::boot, "jsix init args are at: %016lx", args);
    log::debug(logs::boot, "     Memory map is at: %016lx", args->mem_map);
    log::debug(logs::boot, "ACPI root table is at: %016lx", args->acpi_table);
    log::debug(logs::boot, "Runtime service is at: %016lx", args->runtime_services);
    log::debug(logs::boot, "    Kernel PML4 is at: %016lx", args->pml4);

    disable_legacy_pic();

    mem::initialize(*args);

    bsp_late_init();

    device_manager &devices = device_manager::get();
    devices.parse_acpi(args->acpi_table);

    devices.init_drivers();
    cpu_init(cpu, true);

    g_num_cpus = smp::start(*cpu, args->pml4);

    sysconf_create();
    interrupts_enable();

    scheduler *sched = new scheduler {g_num_cpus};

    // Load the init server
    load_init_server(*args->init, args->modules);

    sched->start();
}

void
load_init_server(bootproto::program &program, uintptr_t modules_address)
{
    using bootproto::section_flags;
    using obj::vm_flags;

    obj::process *p = new obj::process;
    p->add_handle(&obj::system::get(), obj::system::init_caps);

    vm_space &space = p->space();
    for (const auto &sect : program.sections) {
        vm_flags flags =
            ((sect.type && section_flags::execute) ? vm_flags::exec : vm_flags::none) |
            ((sect.type && section_flags::write) ? vm_flags::write : vm_flags::none);

        obj::vm_area *vma = new obj::vm_area_fixed(sect.phys_addr, sect.size, flags);
        space.add(sect.virt_addr, vma);
    }

    uint64_t iopl = (3ull << 12);

    obj::thread *main = p->create_thread();
    main->add_thunk_user(program.entrypoint, 0, iopl);
    main->set_state(obj::thread::state::ready);

    // Hacky: No process exists to have created a stack for init; it needs to create
    // its own stack. We take advantage of that to use rsp to pass it the init modules
    // address.
    auto *tcb = main->tcb();
    tcb->rsp3 = modules_address;
}
