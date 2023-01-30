#include <uefi/types.h>
#include <uefi/guid.h>
#include <uefi/tables.h>
#include <uefi/protos/simple_text_output.h>

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include <bootproto/bootconfig.h>
#include <bootproto/kernel.h>
#include <bootproto/memory.h>
#include <util/counted.h>
#include <util/pointers.h>

#include "allocator.h"
#include "bootconfig.h"
#include "console.h"
#include "error.h"
#include "fs.h"
#include "hardware.h"
#include "loader.h"
#include "memory.h"
#include "memory_map.h"
#include "paging.h"
#include "status.h"
#include "video.h"


namespace boot {

/// Change a pointer to point to the higher-half linear-offset version
/// of the address it points to.
template <typename T>
void change_pointer(T *&pointer)
{
    pointer = util::offset_pointer(pointer, bootproto::mem::linear_offset);
}

/// The main procedure for the portion of the loader that runs while
/// UEFI is still in control of the machine. (ie, while the loader still
/// has access to boot services.)
bootproto::args *
uefi_preboot(uefi::handle image, uefi::system_table *st)
{
    uefi::boot_services *bs = st->boot_services;
    uefi::runtime_services *rs = st->runtime_services;

    status_line status {L"Performing UEFI pre-boot"};

    hw::check_cpu_supported();
    memory::init_pointer_fixup(bs, rs);

    bootproto::args *args = new bootproto::args;
    g_alloc.zero(args, sizeof(bootproto::args));

    args->magic = bootproto::args_magic;
    args->version = bootproto::args_version;
    args->runtime_services = rs;
    args->acpi_table = hw::find_acpi_table(st);
    memory::mark_pointer_fixup(&args->runtime_services);

    paging::allocate_tables(args);

    return args;
}

/// Load the kernel and other programs from disk
void
load_resources(bootproto::args *args, video::screen *screen, uefi::handle image, uefi::boot_services *bs)
{
    status_line status {L"Loading programs"};

    fs::file disk = fs::get_boot_volume(image, bs);
    fs::file bc_data = disk.open(L"jsix\\boot.conf");
    bootconfig bc {bc_data.load(), bs};

    args->kernel = loader::load_program(disk, L"kernel", bc.kernel());
    args->init = loader::load_program(disk, L"init server", bc.init());
    args->flags = static_cast<bootproto::boot_flags>(bc.flags());

    loader::load_module(disk, L"initrd", bc.initrd(),
            bootproto::module_type::initrd, 0);

    namespace bits = util::bits;
    using bootproto::desc_flags;

    if (screen) {
        video::make_module(screen);

        // Find the screen-specific panic handler first to
        // give it priority
        for (const descriptor &d : bc.panics()) {
            if (bits::has(d.flags, desc_flags::graphical)) {
                args->panic = loader::load_program(disk, L"panic handler", d);
                break;
            }
        }
    }

    if (!args->panic) {
        for (const descriptor &d : bc.panics()) {
            if (!bits::has(d.flags, desc_flags::graphical)) {
                args->panic = loader::load_program(disk, L"panic handler", d);
                break;
            }
        }
    }

    loader::verify_kernel_header(*args->kernel);
}

memory::efi_mem_map
uefi_exit(bootproto::args *args, uefi::handle image, uefi::boot_services *bs)
{
    status_line status {L"Exiting UEFI", nullptr, false};

    memory::efi_mem_map map;
    map.update(*bs);

    args->mem_map = memory::build_kernel_map(map);
    args->frame_blocks = memory::build_frame_blocks(args->mem_map);

    map.update(*bs);
    status.do_blank();

    try_or_raise(
        bs->exit_boot_services(image, map.key),
        L"Failed to exit boot services");

    return map;
}

} // namespace boot

/// The UEFI entrypoint for the loader.
extern "C" uefi::status
efi_main(uefi::handle image, uefi::system_table *st)
{
    using namespace boot;

    uefi::boot_services *bs = st->boot_services;
    console con(st->con_out);

    bootproto::allocation_register *allocs = nullptr;
    bootproto::modules_page *modules = nullptr;
    memory::allocator::init(allocs, modules, bs);

    video::screen *screen = video::pick_mode(bs);
    con.announce();

    bootproto::args *args = uefi_preboot(image, st);
    load_resources(args, screen, image, bs);
    memory::efi_mem_map map = uefi_exit(args, image, st->boot_services);

    args->allocations = allocs;
    args->modules = reinterpret_cast<uintptr_t>(modules);

    status_bar status {screen}; // Switch to fb status display

    // Map the kernel and panic handler to the appropriate addresses
    paging::map_program(args, *args->kernel);
    paging::map_program(args, *args->panic);

    memory::fix_frame_blocks(args);

    bootproto::entrypoint kentry =
        reinterpret_cast<bootproto::entrypoint>(args->kernel->entrypoint);
    //status.next();

    hw::setup_control_regs();
    memory::virtualize(args->pml4, map, st->runtime_services);
    //status.next();

    change_pointer(args);
    change_pointer(args->pml4);

    change_pointer(args->kernel);
    change_pointer(args->kernel->sections.pointer);
    change_pointer(args->init);
    change_pointer(args->init->sections.pointer);

    //status.next();

    kentry(args);
    debug_break();
    return uefi::status::unsupported;
}

