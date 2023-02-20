#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <j6/cap_flags.h>
#include <j6/errors.h>
#include <j6/init.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <j6/types.h>

#include <bootproto/init.h>
#include <bootproto/devices/framebuffer.h>

#include "j6romfs.h"
#include "loader.h"
#include "modules.h"
#include "service_locator.h"

using bootproto::module;
using bootproto::module_type;

void
load_driver(
    j6romfs::fs &initrd,
    const j6romfs::inode *dir,
    const char *name,
    j6_handle_t sys,
    j6_handle_t slp,
    const module *arg = nullptr)
{
    const j6romfs::inode *in = initrd.lookup_inode_in_dir(dir, name);

    if (in->type != j6romfs::inode_type::file)
        return;

    j6::syslog("Loading driver: %s", name);

    uint8_t *data = new uint8_t [in->size];
    util::buffer program {data, in->size};

    initrd.load_inode_data(in, program);
    load_program(name, program, sys, slp, arg);

    delete [] data;
}

int
driver_main(unsigned argc, const char **argv, const char **env, const j6_init_args *initp)
{
    j6_status_t s;

    j6_handle_t slp_mb = j6_handle_invalid;
    j6_handle_t slp_mb_child = j6_handle_invalid;

    j6_handle_t sys = j6_handle_invalid;
    j6_handle_t sys_child = j6_handle_invalid;

    j6_log("srv.init starting");

    sys = j6_find_first_handle(j6_object_type_system);
    if (sys == j6_handle_invalid)
        return 1;

    s = j6_handle_clone(sys, &sys_child,
            j6_cap_system_bind_irq |
            j6_cap_system_get_log |
            j6_cap_system_map_phys |
            j6_cap_system_change_iopl);
    if (s != j6_status_ok)
        return s;

    s = j6_mailbox_create(&slp_mb);
    if (s != j6_status_ok)
        return s;

    s = j6_handle_clone(slp_mb, &slp_mb_child,
            j6_cap_mailbox_send |
            j6_cap_object_clone);
    if (s != j6_status_ok)
        return s;

    uintptr_t modules_addr = initp->args[0];

    std::vector<const module*> mods;
    load_modules(modules_addr, sys, 0, mods);

    module const *initrd_module = nullptr;
    std::vector<module const*> devices;

    for (auto mod : mods) {
        switch (mod->type) {
        case module_type::initrd:
            initrd_module = mod;
            break;

        case module_type::device:
            devices.push_back(mod);
            break;

        default:
            // Unknown module??
            break;
        }
    }

    if (!initrd_module)
        return 1;

    util::const_buffer initrd_buf = *initrd_module->data<util::const_buffer>();

    j6_handle_t initrd_vma =
        map_phys(sys, initrd_buf.pointer, initrd_buf.count);
    if (initrd_vma == j6_handle_invalid) {
        j6_log("  ** error loading ramdisk: mapping physical vma");
        return 4;
    }

    // TODO: encapsulate this all in a driver_manager, or maybe
    // have driver_source objects..
    j6romfs::fs initrd {initrd_buf};


    const j6romfs::inode *driver_dir = initrd.lookup_inode("/jsix/drivers");
    if (!driver_dir) {
        j6_log("Could not load drivers directory");
        return 1;
    }

    load_driver(initrd, driver_dir, "drv.uart.elf", sys_child, slp_mb_child);
    for (const module *m : devices) {
        switch (m->type_id) {
            case bootproto::devices::type_id_uefi_fb:
                load_driver(initrd, driver_dir, "drv.uefi_fb.elf", sys_child, slp_mb_child, m);
                break;

            default:
                j6::syslog("Unknown device type id: %lx", m->type_id);
        }
    }

    initrd.for_each("/jsix/services",
            [=](const j6romfs::inode *in, const char *name) {
        if (in->type != j6romfs::inode_type::file)
            return;

        j6::syslog("Loading service: %s", name);

        uint8_t *data = new uint8_t [in->size];
        util::buffer program {data, in->size};

        initrd.load_inode_data(in, program);
        load_program(name, program, sys_child, slp_mb_child);

        delete [] data;
    });

    service_locator_start(slp_mb);
    return 0;
}

