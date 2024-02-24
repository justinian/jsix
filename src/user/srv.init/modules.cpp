#include <stdlib.h>
#include <stdio.h>
#include <utility>

#include <arch/memory.h>
#include <j6/errors.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>

#include "loader.h"
#include "modules.h"

void
load_modules(
        uintptr_t address,
        j6_handle_t system,
        j6_handle_t self,
        std::vector<const bootproto::module *> &modules)
{
    using bootproto::module;
    using bootproto::modules_page;

    modules_page const *p = reinterpret_cast<const modules_page*>(address);
    while (p) {
        j6_handle_t mod_vma = map_phys(system, p, arch::frame_size);
        if (mod_vma == j6_handle_invalid) {
            j6::syslog(j6::logs::srv, j6::log_level::error, "error loading modules: mapping physical vma");
            return;
        }

        module const *m = reinterpret_cast<module const*>(p+1);

        while (m->bytes) {
            modules.push_back(m);
            m = util::offset_pointer(m, m->bytes);
        }

        p = p->next;
    }
}

