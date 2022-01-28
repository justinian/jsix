#include <stdio.h>
#include <stdlib.h>

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

    for (auto &mod : mods.of_type(module_type::program)) {
        auto &prog = static_cast<const module_program&>(mod);

        char message[100];
        sprintf(message, "  loading program module '%s' at %lx", prog.filename, prog.base_address);
        j6_log(message);

        if (!load_program(prog, message)) {
            j6_log(message);
            return 1;
        }
    }

    return 0;
}
