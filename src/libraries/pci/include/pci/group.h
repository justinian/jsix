#pragma once
/// \file pci.h
/// PCIe devices and groups

#include <stdint.h>
#include <pci/bus_addr.h>
#include <util/pointers.h>

namespace pci {

struct header_unknown;

/// Represents data about a PCI bus group from the ACPI MCFG
struct group
{
    static constexpr size_t config_size = 0x1000'0000;

    uint16_t group_id;
    uint16_t bus_start;
    uint16_t bus_end;

    header_unknown *base;

    /// Iterator that returns successive valid bus addresses for a group
    struct iterator {
        iterator(const group &g, bus_addr a = {0,0,0});
        iterator(const iterator &i) : m_group {i.m_group}, m_addr {i.m_addr} {}

        inline const bus_addr & operator*() const { return m_addr; }
        inline const bus_addr * operator->() const { return &m_addr; }
        inline bus_addr & operator*() { return m_addr; }
        inline bus_addr * operator->() { return &m_addr; }

        iterator & operator++()  { increment_until_valid(); return *this; }
        iterator operator++(int) { iterator old = *this; increment_until_valid(); return old; }

        friend bool operator==(const iterator &a, const iterator &b) { return a.m_addr == b.m_addr; }
        friend bool operator!=(const iterator &a, const iterator &b) { return a.m_addr != b.m_addr; }

    private:
        void increment();
        void increment_until_valid(bool pre_increment=true);

        const group &m_group;
        bus_addr m_addr;
    };

    iterator begin() const { return iterator {*this, {0, 0, bus_start}}; }
    iterator end() const   { return iterator {*this, {0, 0, uint16_t(bus_end + 1)}}; }

    /// Get the base address of the MMIO configuration registers for a device
    /// \arg addr  The bus address of the device
    /// \returns   A pointer to the MMIO configuration registers
    inline header_unknown* device_base(bus_addr addr) const {
        return util::offset_pointer(base, addr.as_int() << 12);
    }

    /// Check if the given device function is present.
    /// \arg addr  The bus address of the device (relative to this group)
    /// \returns   True if the device function is present
    bool has_device(bus_addr addr) const;
};

} // namespace pci
