#include <new>
#include <stddef.h>
#include <stdint.h>
#include <util/misc.h> // for checksum
#include <util/pointers.h>

#include "assert.h"
#include "acpi_tables.h"
#include "apic.h"
#include "clock.h"
#include "device_manager.h"
#include "interrupts.h"
#include "logger.h"
#include "memory.h"
#include "objects/event.h"


static obj::event * const ignore_event = reinterpret_cast<obj::event*>(-1ull);

static const char expected_signature[] = "RSD PTR ";

device_manager device_manager::s_instance;

struct acpi1_rsdp
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__ ((packed));

struct acpi2_rsdp
{
    char signature[8];
    uint8_t checksum10;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;

    uint32_t length;
    acpi_table_header *xsdt_address;
    uint8_t checksum20;
    uint8_t reserved[3];
} __attribute__ ((packed));

bool
acpi_table_header::validate(uint32_t expected_type) const
{
    if (util::checksum(this, length) != 0) return false;
    return !expected_type || (expected_type == type);
}


device_manager::device_manager() :
    m_lapic_base(0)
{
    m_irqs.ensure_capacity(32);
    m_irqs.set_size(16);
    for (int i = 0; i < 16; ++i)
        m_irqs[i] = {nullptr, 0};

    m_irqs[2] = {ignore_event, 0};
}

template <typename T> static const T *
check_get_table(const acpi_table_header *header)
{
    kassert(header && header->validate(T::type_id), "Invalid ACPI table.");
    return reinterpret_cast<const T *>(header);
}

void
device_manager::parse_acpi(const void *root_table)
{
    kassert(root_table != 0, "ACPI root table pointer is null.");

    const acpi1_rsdp *acpi1 = mem::to_virtual(
        reinterpret_cast<const acpi1_rsdp *>(root_table));

    for (int i = 0; i < sizeof(acpi1->signature); ++i)
        kassert(acpi1->signature[i] == expected_signature[i],
                "ACPI RSDP table signature mismatch");

    uint8_t sum = util::checksum(acpi1, sizeof(acpi1_rsdp), 0);
    kassert(sum == 0, "ACPI 1.0 RSDP checksum mismatch.");

    kassert(acpi1->revision > 1, "ACPI 1.0 not supported.");

    const acpi2_rsdp *acpi2 =
        reinterpret_cast<const acpi2_rsdp *>(acpi1);

    sum = util::checksum(acpi2, sizeof(acpi2_rsdp), sizeof(acpi1_rsdp));
    kassert(sum == 0, "ACPI 2.0 RSDP checksum mismatch.");

    load_xsdt(mem::to_virtual(acpi2->xsdt_address));
}

const device_manager::apic_nmi *
device_manager::get_lapic_nmi(uint8_t id) const
{
    for (const auto &nmi : m_nmis) {
        if (nmi.cpu == 0xff || nmi.cpu == id)
            return &nmi;
    }

    return nullptr;
}

const device_manager::irq_override *
device_manager::get_irq_override(uint8_t irq) const
{
    for (const auto &o : m_overrides)
        if (o.source == irq) return &o;

    return nullptr;
}

ioapic *
device_manager::get_ioapic(int i)
{
    return (i < m_ioapics.count()) ? &m_ioapics[i] : nullptr;
}

static void
put_sig(char *into, uint32_t type)
{
    for (int j=0; j<4; ++j) into[j] = reinterpret_cast<char *>(&type)[j];
}

void
device_manager::load_xsdt(const acpi_table_header *header)
{
    const auto *xsdt = check_get_table<acpi_xsdt>(header);

    char sig[5] = {0,0,0,0,0};
    log::info(logs::device, "ACPI 2.0+ tables loading");

    put_sig(sig, xsdt->header.type);
    log::verbose(logs::device, "  Found table %s", sig);

    size_t num_tables = acpi_table_entries(xsdt, sizeof(void*));
    for (size_t i = 0; i < num_tables; ++i) {
        const acpi_table_header *header =
            mem::to_virtual(xsdt->headers[i]);

        put_sig(sig, header->type);
        log::verbose(logs::device, "  Found table %s", sig);

        kassert(header->validate(), "Table failed validation.");

        switch (header->type) {
        case acpi_apic::type_id:
            load_apic(header);
            break;

        case acpi_mcfg::type_id:
            load_mcfg(header);
            break;

        case acpi_hpet::type_id:
            load_hpet(header);
            break;

        default:
            break;
        }
    }
}

void
device_manager::load_apic(const acpi_table_header *header)
{
    const auto *apic = check_get_table<acpi_apic>(header);

    m_lapic_base = apic->local_address;

    size_t count = acpi_table_entries(apic, 1);
    uint8_t const *p = apic->controller_data;
    uint8_t const *end = p + count;

    // Pass one: count objcts
    unsigned num_lapics = 0;
    unsigned num_ioapics = 0;
    unsigned num_overrides = 0;
    unsigned num_nmis = 0;
    while (p < end) {
        const uint8_t type = p[0];
        const uint8_t length = p[1];

        switch (type) {
        case 0: ++num_lapics; break;
        case 1: ++num_ioapics; break;
        case 2: ++num_overrides; break;
        case 4: ++num_nmis; break;
        default: break;
        }

        p += length;
    }

    m_apic_ids.set_capacity(num_lapics);
    m_ioapics.set_capacity(num_ioapics);
    m_overrides.set_capacity(num_overrides);
    m_nmis.set_capacity(num_nmis);

    // Pass two: configure objects
    p = apic->controller_data;
    while (p < end) {
        const uint8_t type = p[0];
        const uint8_t length = p[1];

        switch (type) {
        case 0: { // Local APIC
                uint8_t uid = util::read_from<uint8_t>(p+2);
                uint8_t id = util::read_from<uint8_t>(p+3);
                m_apic_ids.append(id);

                log::spam(logs::device, "    Local APIC uid %x id %x", uid, id);
            }
            break;

        case 1: { // I/O APIC
                uintptr_t base = util::read_from<uint32_t>(p+4);
                uint32_t base_gsi = util::read_from<uint32_t>(p+8);
                m_ioapics.emplace(base, base_gsi);

                log::spam(logs::device, "    IO APIC gsi %x base %x", base_gsi, base);
            }
            break;

        case 2: { // Interrupt source override
                irq_override o;
                o.source = util::read_from<uint8_t>(p+3);
                o.gsi = util::read_from<uint32_t>(p+4);
                o.flags = util::read_from<uint16_t>(p+8);
                m_overrides.append(o);

                log::spam(logs::device, "    Intr source override IRQ %d -> %d Pol %d Tri %d",
                        o.source, o.gsi, (o.flags & 0x3), ((o.flags >> 2) & 0x3));
            }
            break;

        case 4: {// LAPIC NMI
            apic_nmi nmi;
            nmi.cpu = util::read_from<uint8_t>(p + 2);
            nmi.lint = util::read_from<uint8_t>(p + 5);
            nmi.flags = util::read_from<uint16_t>(p + 3);
            m_nmis.append(nmi);

            log::spam(logs::device, "    LAPIC NMI Proc %02x LINT%d Pol %d Tri %d",
                    nmi.cpu, nmi.lint, nmi.flags & 0x3, (nmi.flags >> 2) & 0x3);
            }
            break;

        default:
            log::spam(logs::device, "    APIC entry type %d", type);
        }

        p += length;
    }

    m_ioapics[0].mask(3, false);
    m_ioapics[0].mask(4, false);
}

void
device_manager::load_mcfg(const acpi_table_header *header)
{
    const auto *mcfg = check_get_table<acpi_mcfg>(header);

    size_t count = acpi_table_entries(mcfg, sizeof(acpi_mcfg_entry));
    m_pci.set_size(count);
    m_devices.set_capacity(16);

    for (unsigned i = 0; i < count; ++i) {
        const acpi_mcfg_entry &mcfge = mcfg->entries[i];

        m_pci[i].group = mcfge.group;
        m_pci[i].bus_start = mcfge.bus_start;
        m_pci[i].bus_end = mcfge.bus_end;
        m_pci[i].base = mem::to_virtual<uint32_t>(mcfge.base);

        log::spam(logs::device, "  Found MCFG entry: base %lx  group %d  bus %d-%d",
                mcfge.base, mcfge.group, mcfge.bus_start, mcfge.bus_end);
    }

    probe_pci();
}

void
device_manager::load_hpet(const acpi_table_header *header)
{
    const auto *hpet = check_get_table<acpi_hpet>(header);

    log::verbose(logs::device, "  Found HPET device #%3d: base %016lx  pmin %d  attr %02x",
            hpet->index, hpet->base_address.address, hpet->periodic_min, hpet->attributes);

    uint32_t hwid = hpet->hardware_id;
    uint8_t rev_id = hwid & 0xff;
    uint8_t comparators = (hwid >> 8) & 0x1f;
    uint8_t count_size_cap = (hwid >> 13) & 1;
    uint8_t legacy_replacement = (hwid >> 15) & 1;
    uint32_t pci_vendor_id = (hwid >> 16);

    log::spam(logs::device, "      rev:%02d comparators:%02d count_size_cap:%1d legacy_repl:%1d",
            rev_id, comparators, count_size_cap, legacy_replacement);
    log::spam(logs::device, "      pci vendor id: %04x", pci_vendor_id);

    m_hpets.emplace(hpet->index,
        reinterpret_cast<uint64_t*>(hpet->base_address.address + mem::linear_offset));
}

void
device_manager::probe_pci()
{
    for (auto &pci : m_pci) {
        log::verbose(logs::device, "Probing PCI group at base %016lx", pci.base);

        for (int bus = pci.bus_start; bus <= pci.bus_end; ++bus) {
            for (int dev = 0; dev < 32; ++dev) {
                if (!pci.has_device(bus, dev, 0)) continue;

                auto &d0 = m_devices.emplace(pci, bus, dev, 0);
                if (!d0.multi()) continue;

                for (int i = 1; i < 8; ++i) {
                    if (pci.has_device(bus, dev, i))
                        m_devices.emplace(pci, bus, dev, i);
                }
            }
        }
    }
}

static uint64_t
tsc_clock_source(void*)
{
    uint32_t lo = 0, hi = 0;
    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

void
device_manager::init_drivers()
{
    // Eventually this should be e.g. a lookup into a loadable driver list
    // for now, just look for AHCI devices
    /*
    for (auto &device : m_devices) {
        if (device.devclass() != 1 || device.subclass() != 6)
            continue;

        if (device.progif() != 1) {
            log::warn(logs::device, "Found SATA device %d:%d:%d, but not an AHCI interface.",
                    device.bus(), device.device(), device.function());
        }

        ahcid.register_device(&device);
    }
    */
    clock *master_clock = nullptr;
    if (m_hpets.count() > 0) {
        hpet &h = m_hpets[0];
        h.enable();

        // becomes the singleton
        master_clock = new clock(h.rate(), hpet_clock_source, &h);
        log::info(logs::clock, "Created master clock using HPET 0: Rate %d", h.rate());
    } else {
        //TODO: Other clocks, APIC clock?
        master_clock = new clock(5000, tsc_clock_source, nullptr);
    }

    kassert(master_clock, "Failed to allocate master clock");
}

bool
device_manager::dispatch_irq(unsigned irq)
{
    if (irq >= m_irqs.count())
        return false;

    irq_binding &binding = m_irqs[irq];
    if (!binding.target || binding.target == ignore_event)
        return binding.target == ignore_event;

    binding.target->signal(1 << binding.signal);
    return true;
}

bool
device_manager::bind_irq(unsigned irq, obj::event *target, unsigned signal)
{
    // TODO: grow if under max size
    if (irq >= m_irqs.count())
        return false;

    m_irqs[irq] = {target, signal};
    return true;
}

void
device_manager::unbind_irqs(obj::event *target)
{
    const size_t count = m_irqs.count();
    for (size_t i = 0; i < count; ++i) {
        if (m_irqs[i].target == target)
            m_irqs[i] = {nullptr, 0};
    }
}

bool
device_manager::allocate_msi(const char *name, pci_device &device, irq_callback cb, void *data)
{
    /*
    // TODO: find gaps to fill
    uint8_t irq = m_irqs.count();
    isr vector = isr::irq00 + irq;
    m_irqs.append({name, cb, data});

    log::spam(logs::device, "Allocating IRQ %02x to %s.", irq, name);

    device.write_msi_regs(
            0xFEE00000,
            static_cast<uint16_t>(vector));
    */
    return true;
}

void
device_manager::register_block_device(block_device *blockdev)
{
    m_blockdevs.append(blockdev);
}
