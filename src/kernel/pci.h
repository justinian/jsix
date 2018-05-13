#pragma once
/// \file pci.h
/// PCI devices and groups
#include <stdint.h>
#include "kutil/memory.h"

struct pci_group;
enum class isr : uint8_t;

struct pci_cap
{
	enum class type : uint8_t
	{
		msi		= 0x05,
		msix	= 0x11
	};

	type id;
	uint8_t next;
} __attribute__ ((packed));


/// Information about a discovered PCIe device
class pci_device
{
public:
	/// Default constructor creates an empty object.
	pci_device();

	/// Constructor
	/// \arg group  The group of this device's bus
	/// \arg bus    The bus number this device is on
	/// \arg device The device number of this device
	/// \arg func   The function number of this device
	pci_device(pci_group &group, uint8_t bus, uint8_t device, uint8_t func);

	/// Check if this device is multi-function.
	/// \returns  True if this device is multi-function
	inline bool multi() const { return m_multi; }

	/// Get the bus number this device is on.
	/// \returns  The bus number
	inline uint8_t bus() const { return (m_bus_addr >> 8); }

	/// Get the device number of this device on its bus
	/// \returns  The device number
	inline uint8_t device() const { return (m_bus_addr >> 3) & 0x1f; }

	/// Get the function number of this device on its device
	/// \returns  The function number
	inline uint8_t function() const { return m_bus_addr & 0x7; }

	/// Get the device class
	/// \returns  The PCI device class
	inline uint8_t devclass() const { return m_class; }

	/// Get the device subclass
	/// \returns  The PCI device subclass
	inline uint8_t subclass() const { return m_subclass; }

	/// Get the device program interface
	/// \returns  The PCI device program interface
	inline uint8_t progif() const { return m_progif; }

	/// Read one of the device's Base Address Registers
	/// \arg i   Which BAR to read (up to 5 for non-bridges)
	/// \returns The contents of the BAR
	uint32_t get_bar(unsigned i);

	/// Write one of the device's Base Address Registers
	/// \arg i   Which BAR to read (up to 5 for non-bridges)
	/// \arg val The value to write
	void set_bar(unsigned i, uint32_t val);

	/// Write to the MSI registers
	/// \arg addr  The address to write to the MSI address registers
	/// \arg data  The value to write to the MSI data register
	void write_msi_regs(addr_t addr, uint16_t data);

	/// Get a bus address, given the bus/device/function numbers.
	/// \arg bus    Number of the bus
	/// \arg device Index of the device on the bus
	/// \arg func   The function number within the device
	/// \returns    The computed bus_addr
	static inline uint16_t bus_addr(uint8_t bus, uint8_t device, uint8_t func)
	{
		return bus << 8 | device << 3 | func;
	}

private:
	uint32_t *m_base;
	pci_cap *m_msi;

	/// Bus address: 15:8 bus, 7:3 device, 2:0 device
	uint16_t m_bus_addr;

	uint16_t m_vendor;
	uint16_t m_device;

	uint8_t m_class;
	uint8_t m_subclass;
	uint8_t m_progif;
	uint8_t m_revision;
	bool m_multi;

	// Might as well cache these to fill out the struct align
	isr m_irq;
	uint8_t m_header_type;
};


/// Represents data about a PCI bus group from the ACPI MCFG
struct pci_group
{
	uint16_t group;
	uint16_t bus_start;
	uint16_t bus_end;

	uint32_t *base;

	/// Get the base address of the MMIO configuration registers for a device
	/// \arg bus    The bus number of the device (relative to this group)
	/// \arg device The device number on the given bus
	/// \arg func   The function number on the device
	/// \returns    A pointer to the memory-mapped configuration registers
	inline uint32_t * base_for(uint8_t bus, uint8_t device, uint8_t func)
	{
		return kutil::offset_pointer(base,
				pci_device::bus_addr(bus, device, func) << 12);
	}

	/// Check if the given device function is present.
	/// \arg bus    The bus number of the device (relative to this group)
	/// \arg device The device number on the given bus
	/// \arg func   The function number on the device
	/// \returns    True if the device function is present
	bool has_device(uint8_t bus, uint8_t device, uint8_t func);
};


