#include <stdint.h>
#include "ahci/ata.h"
#include "ahci/hba.h"
#include "console.h"
#include "device_manager.h"
#include "fs/gpt.h"
#include "log.h"
#include "page_manager.h"
#include "pci.h"


IS_BITFIELD(ahci::hba_cap);
IS_BITFIELD(ahci::hba_cap2);

namespace ahci {


enum class hba_cap : uint32_t
{
	ccc			= 0x00000080,	// Command completion coalescing
	ahci_only	= 0x00040000,	// ACHI-only mode
	clo			= 0x01000000,	// Command list override
	snotify		= 0x40000000,	// SNotification register
	ncq			= 0x40000000,	// Native command queuing
	addr64		= 0x80000000	// 64bit addressing
};


enum class hba_cap2 : uint32_t
{
	handoff		= 0x00000001	// BIOS OS hand-off
};


struct hba_data
{
	hba_cap cap;
	uint32_t host_control;
	uint32_t int_status;
	uint32_t port_impl;
	uint32_t version;
	uint32_t ccc_control;
	uint32_t ccc_ports;
	uint32_t em_location;
	uint32_t em_control;
	hba_cap2 cap2;
	uint32_t handoff_control;

} __attribute__ ((packed));


void irq_cb(void *data)
{
	hba *h = reinterpret_cast<hba *>(data);
	h->handle_interrupt();
}


hba::hba(pci_device *device)
{
	page_manager *pm = page_manager::get();
	device_manager &dm = device_manager::get();

	uint32_t bar5 = device->get_bar(5);
	m_data = reinterpret_cast<hba_data *>(bar5 & ~0xfffull);
	pm->map_offset_pointer(reinterpret_cast<void **>(&m_data), 0x2000);

	if (! bitfield_has(m_data->cap, hba_cap::ahci_only))
		m_data->host_control |= 0x80000000; // Enable AHCI mode

	uint32_t icap = static_cast<uint32_t>(m_data->cap);

	unsigned ports = (icap & 0xf) + 1;
	unsigned slots = ((icap >> 8) & 0x1f) + 1;

	log::debug(logs::driver, "  %d ports", ports);
	log::debug(logs::driver, "  %d command slots", slots);

	port_data *pd = reinterpret_cast<port_data *>(
			kutil::offset_pointer(m_data, 0x100));

	bool needs_interrupt = false;
	m_ports.ensure_capacity(ports);
	for (unsigned i = 0; i < ports; ++i) {
		bool impl = ((m_data->port_impl & (1 << i)) != 0);
		port &p = m_ports.emplace(this, i, kutil::offset_pointer(pd, 0x80 * i), impl);
		if (p.get_state() == port::state::active)
			needs_interrupt = true;
	}

	if (needs_interrupt) {
		device_manager::get().allocate_msi("AHCI Device", *device, irq_cb, this);
		m_data->host_control |= 0x02; // enable interrupts
	}

	for (auto &p : m_ports) {
		if (!p.active()) continue;

		if (p.get_type() == sata_signature::sata_drive) {
			p.identify_async();
			if (fs::partition::load(&p) == 0)
				dm.register_block_device(&p);
		}
	}
}

void
hba::handle_interrupt()
{
	uint32_t status = m_data->int_status;
	for (auto &port : m_ports) {
		if (status & (1 << port.index())) {
			port.handle_interrupt();
		}
	}
	// Write 1 to the handled interrupts
	m_data->int_status = status;
}

void
hba::dump()
{
	console *cons = console::get();
	static const char *regs[] = {
		" CAP", " GHC", "  IS", "  PI", "  VS", " C3C",
		" C3P", " EML", " EMC", "CAP2", "BOHC"
	};

	cons->printf("HBA Registers:\n");
	uint32_t *data = reinterpret_cast<uint32_t *>(m_data);
	for (int i = 0; i < 11; ++i) {
		cons->printf("  %s: %08x\n", regs[i], data[i]);
	}
	cons->putc('\n');
}

} // namespace ahci

