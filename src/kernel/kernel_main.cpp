#include <stdint.h>

#include <j6/memutils.h>
#include <bootproto/kernel.h>
#include <util/vector.h>

#include "kassert.h"
#include "capabilities.h"
#include "cpu.h"
#include "debugcon.h"
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
#include "syscall.h"
#include "sysconf.h"

extern "C" {
    void kernel_main(bootproto::args *args);
}

/// Bootstrap the memory managers.
void load_init_server(bootproto::program &program, uintptr_t modules_address);


void
kernel_main(bootproto::args *args)
{
    if (args->panic_handler) {
        panic::install(args->panic_handler, args->symbol_table);
    }

    cpu_data *cpu = bsp_early_init();
    mem::initialize(*args);

    kassert(args->magic == bootproto::args_magic,
            "Bad kernel args magic number");

    log::verbose(logs::boot, "jsix init args are at: %016lx", args);
    log::verbose(logs::boot, "     Memory map is at: %016lx", args->mem_map);
    log::verbose(logs::boot, "ACPI root table is at: %016lx", args->acpi_table);
    log::verbose(logs::boot, "Runtime service is at: %016lx", args->runtime_services);
    log::verbose(logs::boot, "    Kernel PML4 is at: %016lx", args->pml4);

    disable_legacy_pic();

    bsp_late_init();

    using bootproto::boot_flags;
    bool enable_test = args->flags.get(boot_flags::test);
    syscall_initialize(enable_test);

    device_manager &devices = device_manager::get();
    devices.parse_acpi(args->acpi_table);

    devices.init_drivers();
    cpu_init(cpu, true);

    g_num_cpus = smp::start(*cpu, args->pml4);

    sysconf_create();
    interrupts_enable();

    scheduler *sched = new scheduler {g_num_cpus};
    smp::ready();

    // Initialize the debug console logger (does nothing if not built
    // in debug mode)
    debugcon::init_logger();

    // Load the init server
    load_init_server(args->init, args->init_modules);

    sched->start();
}

void
load_init_server(bootproto::program &program, uintptr_t modules_address)
{
    using bootproto::section_flags;
    using obj::vm_flags;

    obj::process *p = new obj::process {"srv.init"};

    j6_handle_t sys_handle =
        g_cap_table.create(&obj::system::get(), obj::system::init_caps);
    p->add_handle(sys_handle);

    vm_space &space = p->space();
    for (const auto &sect : program.sections) {
        util::bitset32 flags = util::bitset32::of(vm_flags::exact);
        if (sect.type.get(section_flags::execute))
            flags.set(vm_flags::exec);

        if (sect.type.get(section_flags::write))
            flags.set(vm_flags::write);

        obj::vm_area *vma = new obj::vm_area_fixed(sect.phys_addr, sect.size, flags);
        space.add(sect.virt_addr, vma, flags);
    }

    uint64_t iopl = (3ull << 12);

    obj::thread *main = p->create_thread();
    main->add_thunk_user(program.entrypoint, modules_address, 0, 0, iopl);
    main->set_state(obj::thread::state::ready);
}
