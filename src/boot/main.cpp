#include <uefi/types.h>
#include <uefi/guid.h>
#include <uefi/tables.h>
#include <uefi/protos/simple_text_output.h>

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include <bootproto/acpi.h>
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

    return args;
}

/// Load the kernel and other programs from disk
bootproto::entrypoint
load_resources(
        bootproto::args *args,
        video::screen *screen,
        uefi::handle image,
        paging::pager &pager,
        uefi::boot_services *bs)
{
    status_line status {L"Loading programs"};

    fs::file disk = fs::get_boot_volume(image, bs);
    util::buffer bc_data = loader::load_file(disk, L"jsix\\boot.conf");
    bootconfig bc {bc_data, bs};

    util::buffer kernel = loader::load_file(disk, bc.kernel().path);
    uintptr_t kentry = loader::load_program(kernel, L"jsix kernel", pager, true);

    args->flags = bc.flags();

    using bootproto::desc_flags;

    bool has_panic = false;
    util::buffer panic;

    if (screen) {
        video::make_module(screen);

        // Find the screen-specific panic handler first to
        // give it priority
        for (const descriptor &d : bc.panics()) {
            if (d.flags.get(desc_flags::graphical)) {
                panic = loader::load_file(disk, d.path);
                has_panic = true;
                break;
            }
        }
    }

    if (!has_panic) {
        for (const descriptor &d : bc.panics()) {
            if (d.flags.get(desc_flags::graphical)) {
                panic = loader::load_file(disk, d.path);
                has_panic = true;
                break;
            }
        }
    }

    if (has_panic) {
        args->panic_handler = loader::load_program(panic, L"panic handler", pager);

        const wchar_t *symbol_file = bc.symbols();
        if (symbol_file && *symbol_file)
            args->symbol_table = loader::load_file(disk, symbol_file);
    }

    util::buffer init = loader::load_file(disk, bc.init().path);
    loader::parse_program(L"init server", init, args->init);

    loader::load_module(disk, L"initrd", bc.initrd(),
            bootproto::module_type::initrd);

    return reinterpret_cast<bootproto::entrypoint>(kentry);
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

    paging::pager pager {bs};

    bootproto::entrypoint kentry =
        load_resources(args, screen, image, pager, bs);

    bootproto::module *acpi_mod =
        g_alloc.allocate_module(sizeof(bootproto::acpi));
    acpi_mod->type = bootproto::module_type::acpi;
    bootproto::acpi *acpi = acpi_mod->data<bootproto::acpi>();
    acpi->root = args->acpi_table;

    pager.update_kernel_args(args);
    memory::efi_mem_map map = uefi_exit(args, image, st->boot_services);

    for (size_t i = 0; i < args->mem_map.count; ++i) {
        bootproto::mem_entry &e = args->mem_map.pointer[i];
        if (e.type == bootproto::mem_type::acpi) {
            acpi->region = util::buffer::from(e.start, e.pages * memory::page_size);
            break;
        }
    }

    args->allocations = allocs;
    args->init_modules = reinterpret_cast<uintptr_t>(modules);

    status_bar status {screen}; // Switch to fb status display

    memory::fix_frame_blocks(args, pager);

    //status.next();

    hw::setup_control_regs();
    memory::virtualize(pager, map, st->runtime_services);
    //status.next();

    change_pointer(args);
    change_pointer(args->pml4);
    change_pointer(args->symbol_table.pointer);
    change_pointer(args->init.sections.pointer);

    //status.next();

    kentry(args);
    debug_break();
    return uefi::status::unsupported;
}

