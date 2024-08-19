#include <assert.h>

#include <j6/syslog.hh>
#include <pci/device.h>

namespace pci {

struct cap_msi
{
    cap::type id;
    uint8_t next;
    uint16_t control;
} __attribute__ ((packed));

struct cap_msi32
{
    cap::type id;
    uint8_t next;
    uint16_t control;
    uint32_t address;
    uint16_t data;
    uint16_t reserved;
    uint32_t mask;
    uint32_t pending;
} __attribute__ ((packed));

struct cap_msi64
{
    cap::type id;
    uint8_t next;
    uint16_t control;
    uint64_t address;
    uint16_t data;
    uint16_t reserved;
    uint32_t mask;
    uint32_t pending;
} __attribute__ ((packed));

device::device() :
    m_base {nullptr},
    m_bus_addr {0, 0, 0},
    m_vendor {0},
    m_device {0},
    m_class {0},
    m_subclass {0},
    m_progif {0},
    m_revision {0},
    m_header_type {0}
{
}

device::device(bus_addr addr, uint32_t *base) :
    m_base {base},
    m_msi {nullptr},
    m_bus_addr {addr}
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

    j6::syslog(j6::logs::srv, j6::log_level::info,
            "Found PCIe device at %02d:%02d:%d of type %x.%x.%x id %04x:%04x",
            addr.bus, addr.device, addr.function,
            m_class, m_subclass, m_progif, m_vendor, m_device);

    j6::syslog(j6::logs::srv, j6::log_level::verbose, "  = BAR0 %016lld", get_bar(0));
    j6::syslog(j6::logs::srv, j6::log_level::verbose, "  = BAR1 %016lld", get_bar(1));

    if (*status & 0x0010) {
        // Walk the extended capabilities list
        uint8_t next = m_base[13] & 0xff;
        while (next) {
            cap *c = reinterpret_cast<cap *>(util::offset_pointer(m_base, next));
            next = c->next;
            //log::verbose(logs::device, "  - found PCI cap type %02x", c->id);

            if (c->id == cap::type::msi) {
                m_msi = c;
                cap_msi *mcap = reinterpret_cast<cap_msi *>(c);
                mcap->control &= ~0x70; // at most 1 vector allocated
                mcap->control |= 0x01; // Enable interrupts, at most 1 vector allocated
            }
        }
    }
}

uint32_t
device::get_bar(unsigned i)
{
    if ((m_header_type == 0 && i > 5) || // Device max BAR is 5
        (m_header_type == 1 && i > 2))   // Bridge max BAR is 1
        return 0;

    return m_base[4+i];
}

void
device::set_bar(unsigned i, uint32_t val)
{
    if (m_header_type == 0) {
        assert(i < 6); // Device max BAR is 5
    } else {
        assert(m_header_type == 1); // Only device or bridge
        assert(i < 2); // Bridge max BAR is 1
    }

    m_base[4+i] = val;
}

void
device::write_msi_regs(uintptr_t address, uint16_t data)
{
    if (!m_msi)
        return;

    if (m_msi->id == cap::type::msi) {
        cap_msi *mcap = reinterpret_cast<cap_msi *>(m_msi);
        if (mcap->control & 0x0080) {
            cap_msi64 *mcap64 = reinterpret_cast<cap_msi64 *>(m_msi);
            mcap64->address = address;
            mcap64->data = data;
        } else {
            cap_msi32 *mcap32 = reinterpret_cast<cap_msi32 *>(m_msi);
            mcap32->address = address;
            mcap32->data = data;
        }
        uint16_t control = mcap->control;
        control &= 0xff8f; // We're allocating one vector, clear 6::4
        control |= 0x0001; // Enable MSI
        mcap->control = control;
    } else {
        assert(0 && "MIS-X is NYI");
    }
}

/*
void dump_msi(cap_msi *cap)
{
    auto cons = console::get();
    cons->printf("MSI Cap:\n");
    cons->printf("     id: %02x\n", cap->id);
    cons->printf("   next: %02x\n", cap->next);
    cons->printf("control: %04x\n", cap->control);
    if (cap->control & 0x0080) {
        cap_msi64 *cap64 = reinterpret_cast<cap_msi64 *>(cap);
        cons->printf("address: %016x\n", cap64->address);
        cons->printf("   data: %04x\n", cap64->data);
        if (cap->control & 0x100) {
            cons->printf("   mask: %08x\n", cap64->mask);
            cons->printf("pending: %08x\n", cap64->pending);
        }
    } else {
        cap_msi32 *cap32 = reinterpret_cast<cap_msi32 *>(cap);
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

} // namespace pci
