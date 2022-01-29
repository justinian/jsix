#include <stdio.h>
#include <stdlib.h>

#include <j6/caps.h>
#include <j6/errors.h>
#include <j6/syscalls.h>
#include <j6/types.h>
#include <bootproto/init.h>

#include "loader.h"
#include "modules.h"

using bootproto::module;
using bootproto::module_type;
using bootproto::module_program;

extern "C" {
    int main(int, const char **);
}

uintptr_t _arg_modules_phys;   // This gets filled in in _start

extern j6_handle_t __handle_self;
extern j6_handle_t __handle_sys;

int
main(int argc, const char **argv)
{
    j6_log("srv.init starting");

    modules mods = modules::load_modules(_arg_modules_phys, __handle_sys, __handle_self);

    j6_handle_t drv_sys_handle = j6_handle_invalid;
    j6_status_t s = j6_handle_clone(__handle_sys, &drv_sys_handle,
            j6_cap_system_bind_irq | j6_cap_system_map_phys | j6_cap_system_change_iopl);
    if (s != j6_status_ok)
        return s;

    for (auto &mod : mods.of_type(module_type::program)) {
        auto &prog = static_cast<const module_program&>(mod);

        char message[100];
        sprintf(message, "  loading program module '%s' at %lx", prog.filename, prog.base_address);
        j6_log(message);

        if (!load_program(prog, __handle_sys, message)) {
            j6_log(message);
            return 1;
        }
    }

    return 0;
}
