#include "kutil/assert.h"
#include "log.h"
#include "interrupts.h"
#include "pci.h"


pci_device::pci_device() :
	m_base(nullptr),
	m_bus_addr(0),
	m_vendor(0),
	m_device(0),
	m_class(0),
	m_subclass(0),
	m_progif(0),
	m_revision(0),
	m_irq(isr::isrIgnoreF),
	m_header_type(0)
{
}

pci_device::pci_device(pci_group &group, uint8_t bus, uint8_t device, uint8_t func) :
	m_base(group.base_for(bus, device, func)),
	m_bus_addr(bus_addr(bus, device, func)),
	m_irq(isr::isrIgnoreF)
{
	m_vendor = m_base[0] & 0xffff;
	m_device = (m_base[0] >> 16) & 0xffff;

	m_revision = m_base[2] & 0xff;
	m_progif = (m_base[2] >> 8) & 0xff;
	m_subclass = (m_base[2] >> 16) & 0xff;
	m_class = (m_base[2] >> 24) & 0xff;

	m_header_type = (m_base[3] >> 16) & 0x7f;
	m_multi = ((m_base[3] >> 16) & 0x80) == 0x80;

	log::info(logs::device, "Found PCIe device at %02d:%02d:%d of type %d.%d id %04x:%04x",
			bus, device, func, m_class, m_subclass, m_vendor, m_device);
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


bool
pci_group::has_device(uint8_t bus, uint8_t device, uint8_t func)
{
	return (*base_for(bus, device, func) & 0xffff) != 0xffff;
}
