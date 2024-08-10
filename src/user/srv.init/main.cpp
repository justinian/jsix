#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <j6/cap_flags.h>
#include <j6/errors.h>
#include <j6/init.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <j6/thread.hh>
#include <j6/types.h>

#include <bootproto/acpi.h>
#include <bootproto/init.h>
#include <bootproto/devices/framebuffer.h>

#include "acpi.h"
#include "initfs.h"
#include "j6romfs.h"
#include "loader.h"
#include "modules.h"
#include "service_locator.h"

using bootproto::module;
using bootproto::module_type;

constexpr uintptr_t stack_top = 0xf80000000;

int
main(int argc, const char **argv, const char **env)
{
    j6_status_t s;

    // argv[0] is not a char* for init, but the modules pointer
    uintptr_t modules_addr = reinterpret_cast<uintptr_t>(argv[0]);

    j6_handle_t slp_mb = j6_handle_invalid;
    j6_handle_t slp_mb_child = j6_handle_invalid;

    j6_handle_t sys = j6_handle_invalid;
    j6_handle_t sys_child = j6_handle_invalid;

    j6_handle_t vfs_mb = j6_handle_invalid;
    j6_handle_t vfs_mb_child = j6_handle_invalid;

    j6::syslog(j6::logs::srv, j6::log_level::info, "srv.init starting");

    // Since we had no parent to set up a j6_arg_handles object,
    // ask the kernel for our handles and look in that list for
    // the system handle.
    static constexpr size_t num_init_handles = 16;
    j6_handle_descriptor handles[num_init_handles];
    size_t num_handles = num_init_handles;
    s = j6_handle_list(handles, &num_handles);
    if (s != j6_status_ok)
        return s;

    for (unsigned i = 0; i < num_handles; ++i) {
        if (handles[i].type == j6_object_type_system) {
            sys = handles[i].handle;
            break;
        }
    }
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

    s = j6_mailbox_create(&vfs_mb);
    if (s != j6_status_ok)
        return s;

    s = j6_handle_clone(vfs_mb, &vfs_mb_child,
            j6_cap_mailbox_send |
            j6_cap_object_clone);
    if (s != j6_status_ok)
        return s;

    std::vector<const module*> mods;
    load_modules(modules_addr, sys, 0, mods);

    module const *initrd_module = nullptr;
    module const *acpi_module = nullptr;
    std::vector<module const*> devices;

    for (auto mod : mods) {
        switch (mod->type) {
        case module_type::initrd:
            initrd_module = mod;
            break;

        case module_type::acpi:
            acpi_module = mod;
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
        return 2;

    if (!acpi_module)
        return 3;

    util::const_buffer initrd_buf = *initrd_module->data<util::const_buffer>();

    j6_handle_t initrd_vma =
        map_phys(sys, initrd_buf.pointer, initrd_buf.count);
    if (initrd_vma == j6_handle_invalid) {
        j6::syslog(j6::logs::srv, j6::log_level::info, "error loading ramdisk: mapping physical vma");
        return 4;
    }

    // TODO: encapsulate this all in a driver_manager, or maybe
    // have driver_source objects..
    j6romfs::fs initrd {initrd_buf};

    j6::thread vfs_thread {[=, &initrd](){ initfs_start(initrd, vfs_mb); }, stack_top};
    j6_status_t result = vfs_thread.start();

    load_acpi(sys, acpi_module);

    load_program("/jsix/drivers/drv.uart.elf", initrd, sys_child, slp_mb_child, vfs_mb_child);

    for (const module *m : devices) {
        switch (m->type_id) {
            case bootproto::devices::type_id_uefi_fb:
                load_program("/jsix/drivers/drv.uefi_fb.elf", initrd, sys_child, slp_mb_child, vfs_mb_child, m);
                break;

            default:
                j6::syslog(j6::logs::srv, j6::log_level::warn, "Unknown device type id: %lx", m->type_id);
        }
    }

    initrd.for_each("/jsix/services",
            [=](const j6romfs::inode *in, const char *name) {
        if (in->type != j6romfs::inode_type::file)
            return;

        char path [128];
        sprintf(path, "/jsix/services/%s", name);
        load_program(path, initrd, sys_child, slp_mb_child, vfs_mb_child);
   });

    service_locator_start(slp_mb);
    return 0;
}

