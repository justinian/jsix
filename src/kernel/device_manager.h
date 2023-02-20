#pragma once
/// \file device_manager.h
/// The device manager definition

#include <util/vector.h>

#include "apic.h"
#include "hpet.h"

class block_device;

namespace acpi {
    struct table_header;
}

namespace obj {
    class event;
}

using irq_callback = void (*)(void *);


/// Manager for all system hardware devices
class device_manager
{
public:
    /// Constructor.
    device_manager();

    /// Get the system global device manager.
    /// \returns  A reference to the system device manager
    static device_manager & get() { return s_instance; }

    /// Get an IOAPIC
    /// \arg i   Index of the requested IOAPIC
    /// \returns An object representing the given IOAPIC if it exists,
    /// otherwise nullptr.
    ioapic * get_ioapic(int i);

    /// Parse ACPI tables.
    /// \arg root_table  Pointer to the ACPI RSDP
    void parse_acpi(const void *root_table);

    /// Intialize drivers for the current device list.
    void init_drivers();

    /// Bind an IRQ to an endpoint
    /// \arg irq    The IRQ number to bind
    /// \arg target The endpoint to recieve messages when the IRQ is signalled
    /// \arg signal The event's signal number to bind to
    /// \returns    True on success
    bool bind_irq(unsigned irq, obj::event *target, unsigned signal);

    /// Remove IRQ bindings for an endpoint
    /// \arg target The endpoint to remove
    void unbind_irqs(obj::event *target);

    /// Dispatch an IRQ interrupt
    /// \arg irq  The irq number of the interrupt
    /// \returns  True if the interrupt was handled
    bool dispatch_irq(unsigned irq);

    struct apic_nmi
    {
        uint8_t cpu;
        uint8_t lint;
        uint16_t flags;
    };

    struct irq_override
    {
        uint8_t source;
        uint16_t flags;
        uint32_t gsi;
    };

    /// Get the list of APIC ids for other CPUs
    inline const util::vector<uint8_t> & get_apic_ids() const { return m_apic_ids; }

    /// Get the LAPIC base address
    /// \returns The physical base address of the local apic registers
    uintptr_t get_lapic_base() const { return m_lapic_base; }

    /// Get the NMI mapping for the given local APIC
    /// \arg id  ID of the local APIC
    /// \returns apic_nmi structure describing the NMI configuration,
    ///          or null if no configuration was provided
    const apic_nmi * get_lapic_nmi(uint8_t id) const;

    /// Get the IRQ source override for the given IRQ
    /// \arg irq  IRQ number (not isr vector)
    /// \returns  irq_override structure describing that IRQ's
    ///           configuration, or null if no configuration was provided
    const irq_override * get_irq_override(uint8_t irq) const;

    /// Get an HPET device
    /// \arg i    Index of the device to get
    /// \returns  A pointer to the requested device, or nullptr
    inline hpet * get_hpet(unsigned i)
    {
        return i < m_hpets.count() ?
            &m_hpets[i] : nullptr;
    }

private:
    /// Parse the ACPI XSDT and load relevant sub-tables.
    /// \arg xsdt  Pointer to the XSDT from the firmware
    void load_xsdt(const acpi::table_header *xsdt);

    /// Parse the ACPI MADT and initialize APICs from it.
    /// \arg apic  Pointer to the MADT from the XSDT
    void load_apic(const acpi::table_header *apic);

    /// Parse the ACPI HPET and initialize an HPET from it.
    /// \arg hpet  Pointer to the HPET from the XSDT
    void load_hpet(const acpi::table_header *hpet);

    /// Handle a bad IRQ. Called when an interrupt is dispatched
    /// that has no callback.
    void bad_irq(uint8_t irq);

    uintptr_t m_lapic_base;

    util::vector<ioapic> m_ioapics;
    util::vector<hpet> m_hpets;
    util::vector<uint8_t> m_apic_ids;
    util::vector<apic_nmi> m_nmis;
    util::vector<irq_override> m_overrides;

    struct irq_binding
    {
        obj::event *target = nullptr;
        unsigned signal = 0;
    };
    util::vector<irq_binding> m_irqs;

    static device_manager s_instance;

    device_manager(const device_manager &) = delete;
    device_manager operator=(const device_manager &) = delete;
};
