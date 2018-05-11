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

	m_devices.emplace(device);
}

