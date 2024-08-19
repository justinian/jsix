#pragma once
/// \file acpi.h
/// Routines for loading and parsing ACPI tables

#include <stddef.h>
#include <acpi/tables.h>
#include <util/pointers.h>

namespace acpi {

struct system
{
    system(const void* phys, const void *virt);

    /// Iterator for all tables in the system
    struct iterator
    {
        iterator(const table_header *const *addr, ptrdiff_t off) : m_addr {addr}, m_off {off} {};

        operator const table_header *() const   { return m_addr ? offset(*m_addr) : nullptr; }
        const table_header * operator*() const  { return m_addr ? offset(*m_addr) : nullptr; }
        const table_header * operator->() const { return m_addr ? offset(*m_addr) : nullptr; }

        iterator & operator++()  { increment(); return *this; }
        iterator operator++(int) { iterator old = *this; increment(); return old; }

        friend bool operator==(const iterator &a, const iterator &b) { return a.m_addr == b.m_addr; }
        friend bool operator!=(const iterator &a, const iterator &b) { return a.m_addr != b.m_addr; }

    private:
        inline void increment() { if (m_addr) ++m_addr; }

        table_header const * const * m_addr;
        ptrdiff_t m_off;

        inline const table_header * offset(const table_header * addr) const {
            return util::offset_pointer(addr, m_off);
        }
    };

    iterator begin() const;
    iterator end() const;

private:
    const xsdt * get_xsdt() {
        return check_get_table<xsdt>(util::offset_pointer(m_root->xsdt_address, m_offset));
    }

    ptrdiff_t m_offset;
    const rsdp2* m_root;
};

} // namespace acpi
