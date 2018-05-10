#pragma once
/// \file ahci.h
/// AHCI driver and related definitions
#include "kutil/vector.h"

class pci_device;
struct ahci_port_data;


/// A port on an AHCI HBA
class ahci_port
{
public:
	/// Constructor.
	/// \arg data  Pointer to the device's registers for this port
	/// \arg impl  Whether this port is marked as implemented in the HBA
	ahci_port(ahci_port_data *data, bool impl);

	enum class state { unimpl, inactive, active };

	/// Get the current state of this device
	/// \returns  An enum representing the state
	state get_state() const { return m_state; }

	/// Update the state of this object from the register data
	void update();

private:
	ahci_port_data *m_data;
	state m_state;
};


/// An AHCI host bus adapter
class ahci_hba
{
public:
	/// Constructor.
	/// \arg device  The PCI device for this HBA
	ahci_hba(pci_device *device);

private:
	enum class cap : uint32_t;
	enum class cap2 : uint32_t;

	pci_device *m_device;
	uint32_t *m_bara;
	kutil::vector<ahci_port> m_ports;

	cap m_cap;
	cap2 m_cap2;
};


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
	kutil::vector<ahci_hba> m_devices;
};
