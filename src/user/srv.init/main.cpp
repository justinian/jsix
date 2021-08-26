#include <stdio.h>
#include "j6/syscalls.h"
#include "init_args.h"

#include "modules.h"

using kernel::init::module;
using kernel::init::module_type;
using kernel::init::module_program;

extern "C" {
    int main(int, const char **);
}

uintptr_t _arg_modules_phys;   // This gets filled in in _start

j6_handle_t handle_self   = 1; // Self program handle is always 1
j6_handle_t handle_system = 2; // boot protocol is that init gets the system as handle 2

int
main(int argc, const char **argv)
{
    j6_system_log("srv.init starting");

    modules mods = modules::load_modules(_arg_modules_phys, handle_system, handle_self);

    char message[100];
    for (auto &mod : mods.of_type(module_type::program)) {
        auto &prog = static_cast<const module_program&>(mod);
        sprintf(message, "  program module '%s' at %lx", prog.filename, prog.base_address);
        j6_system_log(message);
    }

    return 0;
}
