#include "kutil/enum_bitfields.h"
#include "ahci/driver.h"
#include "log.h"
#include "pci.h"

namespace ahci {


driver::driver()
{
}

void
driver::register_device(pci_device *device)
{
	log::info(logs::driver, "AHCI registering device %d:%d:%d:",
			device->bus(), device->device(), device->function());

	ahci::hba &hba = m_devices.emplace(device);
}

} // namespace
