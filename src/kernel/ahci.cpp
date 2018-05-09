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

ahci_driver::ahci_driver()
{
}

void
ahci_driver::register_device(pci_device *device)
{
	log::info(logs::driver, "AHCI registering device %d:%d:%d:",
			device->bus(), device->device(), device->function());

	uint32_t bar5 = device->get_bar(5);
	uint32_t *bara = reinterpret_cast<uint32_t *>(bar5 & ~0xfffull);

	page_manager *pm = page_manager::get();
	pm->map_offset_pointer(reinterpret_cast<void **>(&bara), 0x2000);

	uint32_t cap = bara[0];

	log::info(logs::driver, "  %d ports", (cap & 0xf) + 1);
	log::info(logs::driver, "  %d command slots", ((cap >> 8) & 0x1f) + 1);
	log::info(logs::driver, "  Supports:");
	if (cap & 0x80000000) log::info(logs::driver, "  - 64 bit addressing");
	if (cap & 0x40000000) log::info(logs::driver, "  - native command queuing");
	if (cap & 0x20000000) log::info(logs::driver, "  - SNotification register");
	if (cap & 0x10000000) log::info(logs::driver, "  - mechanical presence switches");
	if (cap & 0x08000000) log::info(logs::driver, "  - staggered spin up");
	if (cap & 0x04000000) log::info(logs::driver, "  - agressive link power management");
	if (cap & 0x02000000) log::info(logs::driver, "  - activity LED");
	if (cap & 0x01000000) log::info(logs::driver, "  - command list override");
	if (cap & 0x00040000) log::info(logs::driver, "  - AHCI only");
	if (cap & 0x00020000) log::info(logs::driver, "  - port multiplier");
	if (cap & 0x00010000) log::info(logs::driver, "  - FIS switching");
	if (cap & 0x00008000) log::info(logs::driver, "  - PIO multiple DRQ");
	if (cap & 0x00004000) log::info(logs::driver, "  - slumber state");
	if (cap & 0x00002000) log::info(logs::driver, "  - partial state");
	if (cap & 0x00000080) log::info(logs::driver, "  - command completion coalescing");
	if (cap & 0x00000040) log::info(logs::driver, "  - enclosure management");
	if (cap & 0x00000020) log::info(logs::driver, "  - external SATA");
}
