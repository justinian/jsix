#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <j6/cap_flags.h>
#include <j6/errors.h>
#include <j6/init.h>
#include <j6/syscalls.h>
#include <j6/types.h>
#include <bootproto/init.h>

#include "j6romfs.h"
#include "loader.h"
#include "modules.h"
#include "service_locator.h"

using bootproto::module;
using bootproto::module_type;

extern "C" {
    int main(int, const char **);
}

uintptr_t _arg_modules_phys;   // This gets filled in in _start

extern j6_handle_t __handle_self;

util::const_buffer
load_driver_for(const char *name, const j6romfs::fs &initrd)
{
    char driver[256];
    snprintf(driver, sizeof(driver), "/jsix/drivers/drv.%s.elf", name);

    return initrd.load_simple(driver);
}

util::const_buffer
load_service(const char *name, const j6romfs::fs &initrd)
{
    char service[256];
    snprintf(service, sizeof(service), "/jsix/services/srv.%s.elf", name);

    return initrd.load_simple(service);
}

int
main(int argc, const char **argv)
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

    modules mods = modules::load_modules(_arg_modules_phys, sys, __handle_self);

    module const *initrd_module;
    std::vector<module const*> devices;

    for (auto &mod : mods) {
        switch (mod.type) {
        case module_type::initrd:
            initrd_module = &mod;
            break;

        case module_type::device:
            devices.push_back(&mod);
            break;

        default:
            // Unknown module??
            break;
        }
    }

    if (!initrd_module)
        return 1;

    j6_handle_t initrd_vma =
        map_phys(sys, initrd_module->data.pointer, initrd_module->data.count);
    if (initrd_vma == j6_handle_invalid) {
        j6_log("  ** error loading ramdisk: mapping physical vma");
        return 1;
    }

    // TODO: encapsulate this all in a driver_manager, or maybe
    // have driver_source objects..
    j6romfs::fs initrd {initrd_module->data};

    char err_msg [128];

    util::const_buffer uart_elf = load_driver_for("uart", initrd);
    if (uart_elf.pointer) {
        if (!load_program("UART driver", uart_elf, sys_child, slp_mb_child, err_msg)) {
            j6_log(err_msg);
            return 1;
        }
    }

    util::const_buffer logger_elf = load_service("logger", initrd);
    if (uart_elf.pointer) {
        if (!load_program("logger service", logger_elf, sys_child, slp_mb_child, err_msg)) {
            j6_log(err_msg);
            return 1;
        }
    }

    service_locator_start(slp_mb);
    return 0;
}
