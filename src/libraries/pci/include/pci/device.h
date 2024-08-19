#pragma once
/// \file device.h
/// PCIe device

#include <stdint.h>
#include <pci/bus_addr.h>
#include <util/pointers.h>

namespace pci {

struct cap
{
    enum class type : uint8_t
    {
        msi     = 0x05,
        msix    = 0x11
    };

    type id;
    uint8_t next;
} __attribute__ ((packed));


/// Information about a discovered PCIe device
class device
{
public:
    /// Default constructor creates an empty object.
    device();

    /// Constructor
    /// \arg addr  The bus address of this device
    /// \arg base  Base address of this device's config space
    device(bus_addr addr, uint32_t *base);

    /// Check if this device is multi-function.
    /// \returns  True if this device is multi-function
    inline bool multi() const { return m_multi; }

    /// Get the bus address of this device/function
    inline bus_addr addr() const { return m_bus_addr; }

    /// Get the device class
    /// \returns  The PCI device class
    inline uint8_t devclass() const { return m_class; }

    /// Get the device subclass
    /// \returns  The PCI device subclass
    inline uint8_t subclass() const { return m_subclass; }

    /// Get the device program interface
    /// \returns  The PCI device program interface
    inline uint8_t progif() const { return m_progif; }

    /// Read one of the device's Base Address Registers
    /// \arg i   Which BAR to read (up to 5 for non-bridges)
    /// \returns The contents of the BAR
    uint32_t get_bar(unsigned i);

    /// Write one of the device's Base Address Registers
    /// \arg i   Which BAR to read (up to 5 for non-bridges)
    /// \arg val The value to write
    void set_bar(unsigned i, uint32_t val);

    /// Write to the MSI registers
    /// \arg addr  The address to write to the MSI address registers
    /// \arg data  The value to write to the MSI data register
    void write_msi_regs(uintptr_t addr, uint16_t data);

private:
    uint32_t *m_base;
    cap *m_msi;

    bus_addr m_bus_addr;

    uint16_t m_vendor;
    uint16_t m_device;

    uint8_t m_class;
    uint8_t m_subclass;
    uint8_t m_progif;
    uint8_t m_revision;
    bool m_multi;

    uint8_t m_header_type;
};

} // namespace pci
