#include "kutil/enum_bitfields.h"
#include "ahci.h"
#include "log.h"
#include "device_manager.h"
#include "page_manager.h"

enum class ata_status : uint8_t
{
	error		= 0x01,
	index		= 0x02,
	corrected	= 0x04,
	data_ready	= 0x08,
	seek_done	= 0x10,
	fault		= 0x20,
	ready		= 0x40,
	busy		= 0x80
};
IS_BITFIELD(ata_status);

enum class ata_error : uint8_t
{
	amnf	= 0x01, // Address mark not found
	tkznf	= 0x02, // Track 0 not found
	abort	= 0x04, // Command abort
	mcr		= 0x08, // No media
	idnf	= 0x10, // Id not found
	mc		= 0x20, // No media
	unc		= 0x40, // Uncorrectable
	bbk		= 0x80, // Bad sector
};
IS_BITFIELD(ata_error);

enum class ata_cmd : uint8_t
{
	read_pio		= 0x20,
	read_pio_ext	= 0x24,
	read_dma		= 0xC8,
	read_dma_ext	= 0x25,
	write_pio		= 0x30,
	write_pio_ext	= 0x34,
	write_dma		= 0xCA,
	write_dma_ext	= 0x35,
	cache_flush		= 0xE7,
	cache_flush_ext	= 0xEA,
	packet			= 0xA0,
	identify_packet	= 0xA1,
	identify		= 0xEC
};

struct ahci_port_data
{
	uint64_t command_base;
	uint64_t fis_base;
	uint32_t interrupt_status;
	uint32_t interrupt_enable;
	uint32_t command;
	uint32_t reserved;
	uint32_t task_file;
	uint32_t signature;
	uint32_t serial_status;
	uint32_t serial_control;
	uint32_t serial_error;
} __attribute__ ((packed));


ahci_port::ahci_port(ahci_port_data *data, bool impl) :
	m_data(data),
	m_state(state::unimpl)
{
	if (impl) {
		m_state = state::inactive;
		update();
	}
}

void
ahci_port::update()
{
	if (m_state == state::unimpl) return;

	uint32_t detected = m_data->serial_status & 0x0f;
	uint32_t power = (m_data->serial_status >> 8) & 0x0f;

	if (detected == 0x3 && power == 0x1)
		m_state = state::active;
	else
		m_state = state::inactive;

}


enum class ahci_hba::cap : uint32_t
{
	cccs		= 0x00000080,	// Command completion coalescing
	sam			= 0x00040000,	// ACHI-only mode
	sclo		= 0x01000000,	// Command list override
	ssntf		= 0x40000000,	// SNotification register
	sncq		= 0x40000000,	// Native command queuing
	s64a		= 0x80000000	// 64bit addressing
};
IS_BITFIELD(ahci_hba::cap);

enum class ahci_hba::cap2 : uint32_t
{
	boh			= 0x00000001	// BIOS OS hand-off
};
IS_BITFIELD(ahci_hba::cap2);



ahci_hba::ahci_hba(pci_device *device)
{
	page_manager *pm = page_manager::get();

	uint32_t bar5 = device->get_bar(5);
	m_bara = reinterpret_cast<uint32_t *>(bar5 & ~0xfffull);
	pm->map_offset_pointer(reinterpret_cast<void **>(&m_bara), 0x2000);

	ahci_port_data *port_data = reinterpret_cast<ahci_port_data *>(
			kutil::offset_pointer(m_bara, 0x100));

	uint32_t cap_reg = m_bara[0];
	m_cap = static_cast<cap>(cap_reg);
	m_cap2 = static_cast<cap2>(m_bara[6]);

	uint32_t ports = (cap_reg & 0xf) + 1;
	uint32_t slots = ((cap_reg >> 8) & 0x1f) + 1;

	log::info(logs::driver, "  %d ports", ports);
	log::info(logs::driver, "  %d command slots", slots);

	m_ports.ensure_capacity(ports);

	uint32_t pi = m_bara[3];
	for (int i = 0; i < ports; ++i) {
		bool impl = ((pi & (1 << i)) != 0);
		m_ports.emplace(&port_data[i], impl);
	}
}


ahci_driver::ahci_driver()
{
}

void
ahci_driver::register_device(pci_device *device)
{
	log::info(logs::driver, "AHCI registering device %d:%d:%d:",
			device->bus(), device->device(), device->function());

	ahci_hba &hba = m_devices.emplace(device);
}
