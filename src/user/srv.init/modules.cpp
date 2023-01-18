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
        if (++m_idx >= modules_page::per_page) {
            // We've reached the end of a page, see if there's another
            if (!m_page->next) {
                m_page = nullptr;
                break;
            }
            m_idx = 0;
            m_page = reinterpret_cast<modules_page*>(m_page->next);
        }
    }
    while (m_type != type::none && m_type != deref()->type);
    return deref();
}

const module *
module_iterator::operator++(int)
{
    const module *tmp = deref();
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
    const modules_page *first = nullptr;
    while (address) {
        const modules_page *page = load_page(address, system, self);

        char message[100];
        sprintf(message, "srv.init found %d modules from page at 0x%lx", page->count, address);
        j6_log(message);

        if (!first)
            first = page;
        address = page->next;
    }

    return modules {first};
}

