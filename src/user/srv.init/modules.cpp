#include <stdlib.h>
#include <stdio.h>

#include <j6/errors.h>
#include <j6/syscalls.h>

#include "modules.h"

using module = bootproto::module;
using modules_page = bootproto::modules_page;

static const modules_page *
get_page(const module *mod)
{
    return reinterpret_cast<const modules_page*>(
        reinterpret_cast<uintptr_t>(mod) & ~0xfffull);
}

const module *
module_iterator::operator++()
{
    do {
        m_mod = util::offset_pointer(m_mod, m_mod->mod_length);

        if (m_mod->mod_type == type::none) {
            // We've reached the end of a page, see if there's another
            const modules_page *page = get_page(m_mod);
            if (!page->next) {
                m_mod = nullptr;
                break;
            }

            m_mod = page->modules;
        }
    }
    while (m_type != type::none && m_type != m_mod->mod_type);

    return m_mod;
}

const module *
module_iterator::operator++(int)
{
    const module *tmp = m_mod;
    operator++();
    return tmp;
}

const modules_page *
load_page(uintptr_t address, j6_handle_t system, j6_handle_t self)
{
    j6_handle_t mods_vma = j6_handle_invalid;
    j6_status_t s = j6_system_map_phys(system, &mods_vma, address, 0x1000, 0);
    if (s != j6_status_ok)
        exit(s);

    s = j6_vma_map(mods_vma, self, address);
    if (s != j6_status_ok)
        exit(s);

    return reinterpret_cast<const modules_page*>(address);
}

modules
modules::load_modules(uintptr_t address, j6_handle_t system, j6_handle_t self)
{
    const module *first = nullptr;
    while (address) {
        const modules_page *page = load_page(address, system, self);

        char message[100];
        sprintf(message, "srv.init found %d modules from page at 0x%lx", page->count, address);
        j6_log(message);

        if (!first)
            first = page->modules;
        address = page->next;
    }

    return modules {first};
}

