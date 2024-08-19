#include <stddef.h>
#include <stdint.h>
#include <util/misc.h> // for checksum
#include <util/pointers.h>
#include <acpi/tables.h>

#include "kassert.h"
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

device_manager::device_manager() :
    m_lapic_base(0)
{
    m_irqs.ensure_capacity(32);
    m_irqs.set_size(16);
    for (int i = 0; i < 16; ++i)
        m_irqs[i] = {nullptr, 0};

    m_irqs[2] = {ignore_event, 0};
}

void
device_manager::parse_acpi(const void *root_table)
{
    kassert(root_table != 0, "ACPI root table pointer is null.");

    const acpi::rsdp1 *acpi1 = mem::to_virtual(
        reinterpret_cast<const acpi::rsdp1 *>(root_table));

    for (int i = 0; i < sizeof(acpi1->signature); ++i)
        kassert(acpi1->signature[i] == expected_signature[i],
                "ACPI RSDP table signature mismatch");

    uint8_t sum = util::checksum(acpi1, sizeof(acpi::rsdp1), 0);
    kassert(sum == 0, "ACPI 1.0 RSDP checksum mismatch.");

    kassert(acpi1->revision > 1, "ACPI 1.0 not supported.");

    const acpi::rsdp2 *acpi2 =
        reinterpret_cast<const acpi::rsdp2 *>(acpi1);

    sum = util::checksum(acpi2, sizeof(acpi::rsdp2), sizeof(acpi::rsdp1));
    kassert(sum == 0, "ACPI 2.0 RSDP checksum mismatch.");

    const acpi::table_header *xsdt = mem::to_virtual(acpi2->xsdt_address);
    kassert(xsdt->validate(), "Bad XSDT table");
    load_xsdt(xsdt);
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
device_manager::load_xsdt(const acpi::table_header *header)
{
    const auto *xsdt = acpi::check_get_table<acpi::xsdt>(header);

    char sig[5] = {0,0,0,0,0};
    log::info(logs::device, "ACPI 2.0+ tables loading");

    put_sig(sig, xsdt->header.type);
    log::verbose(logs::device, "  Loading table %s", sig);

    size_t num_tables = acpi::table_entries(xsdt, sizeof(void*));
    for (size_t i = 0; i < num_tables; ++i) {
        const acpi::table_header *header =
            mem::to_virtual(xsdt->headers[i]);

        kassert(header->validate(), "Table failed validation.");
        put_sig(sig, header->type);

        switch (header->type) {
        case acpi::apic::type_id:
            log::verbose(logs::device, "  Loading table %s", sig);
            load_apic(header);
            break;

        case acpi::hpet::type_id:
            log::verbose(logs::device, "  Loading table %s", sig);
            load_hpet(header);
            break;

        default:
            log::verbose(logs::device, "  Skipping table %s", sig);
            break;
        }
    }
}

void
device_manager::load_apic(const acpi::table_header *header)
{
    const auto *apic = acpi::check_get_table<acpi::apic>(header);

    m_lapic_base = apic->local_address;

    size_t count = acpi::table_entries(apic, 1);
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
device_manager::load_hpet(const acpi::table_header *header)
{
    const auto *hpet = acpi::check_get_table<acpi::hpet>(header);

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
        log::info(logs::timer, "Created master clock using HPET 0: Rate %d", h.rate());
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

    binding.target->signal(1ull << binding.signal);
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

/*
bool
device_manager::allocate_msi(const char *name, pci_device &device, irq_callback cb, void *data)
{
    // TODO: find gaps to fill
    uint8_t irq = m_irqs.count();
    isr vector = isr::irq00 + irq;
    m_irqs.append({name, cb, data});

    log::spam(logs::device, "Allocating IRQ %02x to %s.", irq, name);

    device.write_msi_regs(
            0xFEE00000,
            static_cast<uint16_t>(vector));
    return true;
}
*/
