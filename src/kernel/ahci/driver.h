#pragma once
/// \file ahci.h
/// AHCI driver and related definitions
#include "kutil/vector.h"
#include "ahci/hba.h"

class pci_device;


/// Basic AHCI driver
class ahci_driver
{
public:
	/// Constructor.
	ahci_driver();

	/// Register a device with the driver
	/// \arg device  The PCI device to handle
	void register_device(pci_device *device);

	/// Unregister a device from the driver
	/// \arg device  The PCI device to remove
	void unregister_device(pci_device *device);

private:
	kutil::vector<ahci::hba> m_devices;
};

