#include <stddef.h>
#include <stdint.h>

#include "kutil/assert.h"
#include "kutil/memory.h"
#include "acpi_tables.h"
#include "ahci/driver.h"
#include "apic.h"
#include "console.h"
#include "device_manager.h"
#include "interrupts.h"
#include "log.h"
#include "memory.h"
#include "page_manager.h"


static const char expected_signature[] = "RSD PTR ";

device_manager device_manager::s_instance(nullptr);
ahci_driver ahcid;

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
	uint64_t xsdt_address;
	uint8_t checksum20;
	uint8_t reserved[3];
} __attribute__ ((packed));

uint8_t
acpi_checksum(const void *p, size_t len, size_t off = 0)
{
	uint8_t sum = 0;
	const uint8_t *c = reinterpret_cast<const uint8_t *>(p);
	for (int i = off; i < len; ++i) sum += c[i];
	return sum;
}

bool
acpi_table_header::validate(uint32_t expected_type) const
{
	if (acpi_checksum(this, length) != 0) return false;
	return !expected_type || (expected_type == type);
}


void irq2_callback(void *)
{
	console *cons = console::get();
	cons->set_color(11);
	cons->puts(".");
	cons->set_color();
}

void irq4_callback(void *)
{
	// TODO: move this to a real serial driver
	console *cons = console::get();
	cons->echo();
}


device_manager::device_manager(const void *root_table) :
	m_lapic(nullptr)
{
	kassert(root_table != 0, "ACPI root table pointer is null.");

	const acpi1_rsdp *acpi1 =
		reinterpret_cast<const acpi1_rsdp *>(root_table);

	for (int i = 0; i < sizeof(acpi1->signature); ++i)
		kassert(acpi1->signature[i] == expected_signature[i],
				"ACPI RSDP table signature mismatch");

	uint8_t sum = acpi_checksum(acpi1, sizeof(acpi1_rsdp), 0);
	kassert(sum == 0, "ACPI 1.0 RSDP checksum mismatch.");

	kassert(acpi1->revision > 1, "ACPI 1.0 not supported.");

	const acpi2_rsdp *acpi2 =
		reinterpret_cast<const acpi2_rsdp *>(acpi1);

	sum = acpi_checksum(acpi2, sizeof(acpi2_rsdp), sizeof(acpi1_rsdp));
	kassert(sum == 0, "ACPI 2.0 RSDP checksum mismatch.");

	load_xsdt(reinterpret_cast<const acpi_xsdt *>(acpi2->xsdt_address));

	m_irqs.ensure_capacity(32);
	m_irqs.set_size(16);
	m_irqs[2] = {"Clock interrupt", irq2_callback, nullptr};
	m_irqs[4] = {"Serial interrupt", irq4_callback, nullptr};
}

ioapic *
device_manager::get_ioapic(int i)
{
	return (i < m_ioapics.count()) ? m_ioapics[i] : nullptr;
}

static void
put_sig(char *into, uint32_t type)
{
	for (int j=0; j<4; ++j) into[j] = reinterpret_cast<char *>(&type)[j];
}

void
device_manager::load_xsdt(const acpi_xsdt *xsdt)
{
	kassert(xsdt && acpi_validate(xsdt), "Invalid ACPI XSDT.");

	char sig[5] = {0,0,0,0,0};
	log::info(logs::device, "ACPI 2.0+ tables loading");

	put_sig(sig, xsdt->header.type);
	log::debug(logs::device, "  Found table %s", sig);

	size_t num_tables = acpi_table_entries(xsdt, sizeof(void*));
	for (size_t i = 0; i < num_tables; ++i) {
		const acpi_table_header *header = xsdt->headers[i];

		put_sig(sig, header->type);
		log::debug(logs::device, "  Found table %s", sig);

		kassert(header->validate(), "Table failed validation.");

		switch (header->type) {
		case acpi_apic::type_id:
			load_apic(reinterpret_cast<const acpi_apic *>(header));
			break;

		case acpi_mcfg::type_id:
			load_mcfg(reinterpret_cast<const acpi_mcfg *>(header));
			break;

		default:
			break;
		}
	}
}

void
device_manager::load_apic(const acpi_apic *apic)
{
	uint32_t *local = reinterpret_cast<uint32_t *>(apic->local_address);

	m_lapic = new lapic(local, isr::isrSpurious);

	size_t count = acpi_table_entries(apic, 1);
	uint8_t const *p = apic->controller_data;
	uint8_t const *end = p + count;

	// Pass one: set up IOAPIC objcts
	while (p < end) {
		const uint8_t type = p[0];
		const uint8_t length = p[1];
		if (type == 1) {
			uint32_t *base = reinterpret_cast<uint32_t *>(kutil::read_from<uint32_t>(p+4));
			uint32_t base_gsr = kutil::read_from<uint32_t>(p+8);
			m_ioapics.append(new ioapic(base, base_gsr));
		}
		p += length;
	}

	// Pass two: configure APIC objects
	p = apic->controller_data;
	while (p < end) {
		const uint8_t type = p[0];
		const uint8_t length = p[1];

		switch (type) {
		case 0: // Local APIC
		case 1: // I/O APIC
			break;

		case 2: { // Interrupt source override
				uint8_t source = kutil::read_from<uint8_t>(p+3);
				isr gsi = isr::irq00 + kutil::read_from<uint32_t>(p+4);
				uint16_t flags = kutil::read_from<uint16_t>(p+8);

				log::debug(logs::device, "    Intr source override IRQ %d -> %d Pol %d Tri %d",
						source, gsi, (flags & 0x3), ((flags >> 2) & 0x3));

				// TODO: in a multiple-IOAPIC system this might be elsewhere
				m_ioapics[0]->redirect(source, static_cast<isr>(gsi), flags, true);
			}
			break;

		case 4: {// LAPIC NMI
			uint8_t cpu = kutil::read_from<uint8_t>(p + 2);
			uint8_t num = kutil::read_from<uint8_t>(p + 5);
			uint16_t flags = kutil::read_from<uint16_t>(p + 3);

			log::debug(logs::device, "    LAPIC NMI Proc %d LINT%d Pol %d Tri %d",
					kutil::read_from<uint8_t>(p+2),
					kutil::read_from<uint8_t>(p+5),
					kutil::read_from<uint16_t>(p+3) & 0x3,
					(kutil::read_from<uint16_t>(p+3) >> 2) & 0x3);

			m_lapic->enable_lint(num, num == 0 ? isr::isrLINT0 : isr::isrLINT1, true, flags);
			}
			break;

		default:
			log::debug(logs::device, "    APIC entry type %d", type);
		}

		p += length;
	}

	// m_lapic->enable_timer(isr::isrTimer, 128, 3000000);

	for (uint8_t i = 0; i < m_ioapics[0]->get_num_gsi(); ++i) {
		switch (i) {
			case 2: break;
			default: m_ioapics[0]->mask(i, false);
		}
	}

	m_ioapics[0]->dump_redirs();
	m_lapic->enable();
}

void
device_manager::load_mcfg(const acpi_mcfg *mcfg)
{
	size_t count = acpi_table_entries(mcfg, sizeof(acpi_mcfg_entry));
	m_pci.set_size(count);
	m_devices.set_capacity(16);

	page_manager *pm = page_manager::get();

	for (unsigned i = 0; i < count; ++i) {
		const acpi_mcfg_entry &mcfge = mcfg->entries[i];

		m_pci[i].group = mcfge.group;
		m_pci[i].bus_start = mcfge.bus_start;
		m_pci[i].bus_end = mcfge.bus_end;
		m_pci[i].base = reinterpret_cast<uint32_t *>(mcfge.base);

		int num_busses = m_pci[i].bus_end - m_pci[i].bus_start + 1;

		/// Map the MMIO space into memory
		pm->map_offset_pointer(reinterpret_cast<void **>(&m_pci[i].base),
				(num_busses << 20));

		log::debug(logs::device, "  Found MCFG entry: base %lx  group %d  bus %d-%d",
				mcfge.base, mcfge.group, mcfge.bus_start, mcfge.bus_end);
	}

	probe_pci();
}

void
device_manager::probe_pci()
{
	for (auto &pci : m_pci) {
		log::debug(logs::device, "Probing PCI group at base %016lx", pci.base);

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

void
device_manager::init_drivers()
{
	// Eventually this should be e.g. a lookup into a loadable driver list
	// for now, just look for AHCI devices
	for (auto &device : m_devices) {
		if (device.devclass() != 1 || device.subclass() != 6)
			continue;

		if (device.progif() != 1) {
			log::warn(logs::device, "Found SATA device %d:%d:%d, but not an AHCI interface.",
					device.bus(), device.device(), device.function());
		}

		ahcid.register_device(&device);
	}
}

bool
device_manager::allocate_msi(const char *name, pci_device &device, irq_callback cb, void *data)
{
	// TODO: find gaps to fill
	uint8_t irq = m_irqs.count();
	isr vector = isr::irq00 + irq;
	m_irqs.append({name, cb, data});

	log::debug(logs::device, "Allocating IRQ %02x to %s.", irq, name);

	device.write_msi_regs(
			0xFEE00000,
			static_cast<uint16_t>(vector));
	return true;
}
