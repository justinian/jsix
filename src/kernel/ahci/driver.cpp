#include "kutil/enum_bitfields.h"
#include "ahci/driver.h"
#include "log.h"
#include "pci.h"


ahci_driver::ahci_driver()
{
}

void
ahci_driver::register_device(pci_device *device)
{
	log::info(logs::driver, "AHCI registering device %d:%d:%d:",
			device->bus(), device->device(), device->function());

	ahci::hba &hba = m_devices.emplace(device);
}

ahci::port *
ahci_driver::find_disk()
{
	for (auto &hba : m_devices) {
		ahci::port *d = hba.find_disk();
		if (d) return d;
	}
	return nullptr;
}
