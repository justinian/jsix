#include <stddef.h>
#include <stdint.h>

#include "kutil/memory.h"
#include "acpi_tables.h"
#include "apic.h"
#include "assert.h"
#include "device_manager.h"
#include "interrupts.h"
#include "log.h"
#include "memory.h"
#include "memory_pages.h"

static const char expected_signature[] = "RSD PTR ";

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

struct pci_group
{
	uint16_t group;
	uint16_t bus_start;
	uint16_t bus_end;

	uint32_t *base;
};

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

device_manager::device_manager(const void *root_table) :
	m_lapic(nullptr),
	m_num_ioapics(0),
	m_pci(nullptr),
	m_num_pci_groups(0),
	m_devices(nullptr),
	m_num_devices(0),
	m_num_device_entries(0)
{
	kassert(root_table != 0, "ACPI root table pointer is null.");

	kutil::memset(m_ioapics, 0, sizeof(m_ioapics));

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
}

ioapic *
device_manager::get_ioapic(int i)
{
	return (i < m_num_ioapics) ? m_ioapics[i] : nullptr;
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
	log::info(logs::devices, "ACPI 2.0+ tables loading");

	put_sig(sig, xsdt->header.type);
	log::debug(logs::devices, "  Found table %s", sig);

	size_t num_tables = acpi_table_entries(xsdt, sizeof(void*));
	for (size_t i = 0; i < num_tables; ++i) {
		const acpi_table_header *header = xsdt->headers[i];

		put_sig(sig, header->type);
		log::debug(logs::devices, "  Found table %s", sig);

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
			m_ioapics[m_num_ioapics++] = new ioapic(base, base_gsr);
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

				log::debug(logs::devices, "    Intr source override IRQ %d -> %d Pol %d Tri %d",
						source, gsi, (flags & 0x3), ((flags >> 2) & 0x3));

				// TODO: in a multiple-IOAPIC system this might be elsewhere
				m_ioapics[0]->redirect(source, static_cast<isr>(gsi), flags, true);
			}
			break;

		case 4: {// LAPIC NMI
			uint8_t cpu = kutil::read_from<uint8_t>(p + 2);
			uint8_t num = kutil::read_from<uint8_t>(p + 5);
			uint16_t flags = kutil::read_from<uint16_t>(p + 3);

			log::debug(logs::devices, "    LAPIC NMI Proc %d LINT%d Pol %d Tri %d",
					kutil::read_from<uint8_t>(p+2),
					kutil::read_from<uint8_t>(p+5),
					kutil::read_from<uint16_t>(p+3) & 0x3,
					(kutil::read_from<uint16_t>(p+3) >> 2) & 0x3);

			m_lapic->enable_lint(num, num == 0 ? isr::isrLINT0 : isr::isrLINT1, true, flags);
			}
			break;

		default:
			log::debug(logs::devices, "    APIC entry type %d", type);
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

static uint32_t *
base_for(uint32_t *group, int bus, int dev, int func)
{
	return kutil::offset_pointer(group,
			(bus << 20) + (dev << 15) + (func << 12));
}

static bool
check_function(uint32_t *group, int bus, int dev, int func)
{
	uint32_t *base = base_for(group, bus, dev, func);

	uint32_t vendor = base[0] & 0xffff;
	uint32_t device = (base[0] >> 16) & 0xffff;
	if (vendor == 0xffff) return false;

	uint32_t revision = base[2] & 0xff;
	uint32_t subclass = (base[2] >> 16) & 0xff;
	uint32_t devclass = (base[2] >> 24) & 0xff;

	uint32_t header = (base[3] >> 16) & 0x7f;
	bool multi = ((base[3] >> 16) & 0x80) == 0x80;

	log::info(logs::devices, "Found PCIe device at %02d:%02d:%d of type %d.%d id %04x:%04x",
			bus, dev, func, devclass, subclass, vendor, device);

	if (header == 0) {
		log::debug(logs::devices, "  Interrupt: %d", (base[15] >> 8) & 0xff);
	}

	return multi;
}

static void
check_device(uint32_t *group, int bus, int dev)
{
	if (check_function(group, bus, dev, 0)) {
		for (int i = 1; i < 8; ++i)
			check_function(group, bus, dev, i);
	}
}

static void
enumerate_devices(pci_group &group)
{
	for (int bus = group.bus_start; bus <= group.bus_end; ++bus) {
		for (int dev = 0; dev < 32; ++dev) {
			check_device(group.base, bus - group.bus_start, dev);
		}
	}
}

void
device_manager::load_mcfg(const acpi_mcfg *mcfg)
{
	size_t count = acpi_table_entries(mcfg, sizeof(acpi_mcfg_entry));

	m_pci = new pci_group[count];
	m_num_pci_groups = count;

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

		log::debug(logs::devices, "  Found MCFG entry: base %lx  group %d  bus %d-%d",
				mcfge.base, mcfge.group, mcfge.bus_start, mcfge.bus_end);
		enumerate_devices(m_pci[i]);
	}
}
