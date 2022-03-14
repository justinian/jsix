#include "assert.h"
#include "logger.h"
#include "interrupts.h"
#include "pci.h"

struct pci_cap_msi
{
    pci_cap::type id;
    uint8_t next;
    uint16_t control;
} __attribute__ ((packed));

struct pci_cap_msi32
{
    pci_cap::type id;
    uint8_t next;
    uint16_t control;
    uint32_t address;
    uint16_t data;
    uint16_t reserved;
    uint32_t mask;
    uint32_t pending;
} __attribute__ ((packed));

struct pci_cap_msi64
{
    pci_cap::type id;
    uint8_t next;
    uint16_t control;
    uint64_t address;
    uint16_t data;
    uint16_t reserved;
    uint32_t mask;
    uint32_t pending;
} __attribute__ ((packed));


/*
void dump_msi(pci_cap_msi *cap)
{
    auto cons = console::get();
    cons->printf("MSI Cap:\n");
    cons->printf("     id: %02x\n", cap->id);
    cons->printf("   next: %02x\n", cap->next);
    cons->printf("control: %04x\n", cap->control);
    if (cap->control & 0x0080) {
        pci_cap_msi64 *cap64 = reinterpret_cast<pci_cap_msi64 *>(cap);
        cons->printf("address: %016x\n", cap64->address);
        cons->printf("   data: %04x\n", cap64->data);
        if (cap->control & 0x100) {
            cons->printf("   mask: %08x\n", cap64->mask);
            cons->printf("pending: %08x\n", cap64->pending);
        }
    } else {
        pci_cap_msi32 *cap32 = reinterpret_cast<pci_cap_msi32 *>(cap);
        cons->printf("address: %08x\n", cap32->address);
        cons->printf("   data: %04x\n", cap32->data);
        if (cap->control & 0x100) {
            cons->printf("   mask: %08x\n", cap32->mask);
            cons->printf("pending: %08x\n", cap32->pending);
        }
    }
    cons->putc('\n');
};
*/

pci_device::pci_device() :
    m_base(nullptr),
    m_bus_addr(0),
    m_vendor(0),
    m_device(0),
    m_class(0),
    m_subclass(0),
    m_progif(0),
    m_revision(0),
    m_irq(isr::isrIgnore0),
    m_header_type(0)
{
}

pci_device::pci_device(pci_group &group, uint8_t bus, uint8_t device, uint8_t func) :
    m_base(group.base_for(bus, device, func)),
    m_msi(nullptr),
    m_bus_addr(bus_addr(bus, device, func)),
    m_irq(isr::isrIgnore0)
{
    m_vendor = m_base[0] & 0xffff;
    m_device = (m_base[0] >> 16) & 0xffff;

    m_revision = m_base[2] & 0xff;
    m_progif = (m_base[2] >> 8) & 0xff;
    m_subclass = (m_base[2] >> 16) & 0xff;
    m_class = (m_base[2] >> 24) & 0xff;

    m_header_type = (m_base[3] >> 16) & 0x7f;
    m_multi = ((m_base[3] >> 16) & 0x80) == 0x80;

    uint16_t *command = reinterpret_cast<uint16_t *>(&m_base[1]);
    *command |= 0x400; // Mask old INTx style interrupts

    uint16_t *status = command + 1;

    log::info(logs::device, "Found PCIe device at %02d:%02d:%d of type %x.%x.%x id %04x:%04x",
            bus, device, func, m_class, m_subclass, m_progif, m_vendor, m_device);

    if (*status & 0x0010) {
        // Walk the extended capabilities list
        uint8_t next = m_base[13] & 0xff;
        while (next) {
            pci_cap *cap = reinterpret_cast<pci_cap *>(util::offset_pointer(m_base, next));
            next = cap->next;
            log::debug(logs::device, "  - found PCI cap type %02x", cap->id);

            if (cap->id == pci_cap::type::msi) {
                m_msi = cap;
                pci_cap_msi *mcap = reinterpret_cast<pci_cap_msi *>(cap);
                mcap->control &= ~0x70; // at most 1 vector allocated
                mcap->control |= 0x01; // Enable interrupts, at most 1 vector allocated
            }
        }
    }
}

uint32_t
pci_device::get_bar(unsigned i)
{
    if (m_header_type == 0) {
        kassert(i < 6, "Requested BAR >5 for PCI device");
    } else if (m_header_type == 1) {
        kassert(i < 2, "Requested BAR >1 for PCI bridge");
    } else {
        kassert(0, "Requested BAR for other PCI device type");
    }

    return m_base[4+i];
}

void
pci_device::set_bar(unsigned i, uint32_t val)
{
    if (m_header_type == 0) {
        kassert(i < 6, "Requested BAR >5 for PCI device");
    } else if (m_header_type == 1) {
        kassert(i < 2, "Requested BAR >1 for PCI bridge");
    } else {
        kassert(0, "Requested BAR for other PCI device type");
    }

    m_base[4+i] = val;
}

void
pci_device::write_msi_regs(uintptr_t address, uint16_t data)
{
    kassert(m_msi, "Tried to write MSI for a device without that cap");
    if (m_msi->id == pci_cap::type::msi) {
        pci_cap_msi *mcap = reinterpret_cast<pci_cap_msi *>(m_msi);
        if (mcap->control & 0x0080) {
            pci_cap_msi64 *mcap64 = reinterpret_cast<pci_cap_msi64 *>(m_msi);
            mcap64->address = address;
            mcap64->data = data;
        } else {
            pci_cap_msi32 *mcap32 = reinterpret_cast<pci_cap_msi32 *>(m_msi);
            mcap32->address = address;
            mcap32->data = data;
        }
        uint16_t control = mcap->control;
        control &= 0xff8f; // We're allocating one vector, clear 6::4
        control |= 0x0001; // Enable MSI
        mcap->control = control;
    } else {
        kassert(0, "MIS-X is NYI");
    }
}

bool
pci_group::has_device(uint8_t bus, uint8_t device, uint8_t func)
{
    return (*base_for(bus, device, func) & 0xffff) != 0xffff;
}
