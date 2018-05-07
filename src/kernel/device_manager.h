#pragma once
/// \file device_manager.h
/// The device manager and related device classes.
#include <stdint.h>

struct acpi_xsdt;
struct acpi_apic;
struct acpi_mcfg;

class lapic;
class ioapic;
class pci_device;
struct pci_group;

enum class isr : uint8_t;


/// Manager for all system hardware devices
class device_manager
{
public:
	/// Constructor.
	/// \arg root_table   Pointer to the ACPI RSDP
	device_manager(const void *root_table);

	/// Get the LAPIC
	/// \returns An object representing the local APIC
	lapic * get_lapic() { return m_lapic; }

	/// Get an IOAPIC
	/// \arg i   Index of the requested IOAPIC
	/// \returns An object representing the given IOAPIC if it exists,
	/// otherwise nullptr.
	ioapic * get_ioapic(int i);

private:
	/// Parse the ACPI XSDT and load relevant sub-tables.
	/// \arg xsdt  Pointer to the XSDT from the firmware
	void load_xsdt(const acpi_xsdt *xsdt);

	/// Parse the ACPI MADT and initialize APICs from it.
	/// \arg apic  Pointer to the MADT from the XSDT
	void load_apic(const acpi_apic *apic);

	/// Parse the ACPI MCFG and initialize PCIe from it.
	/// \arg mcfg  Pointer to the MCFG from the XSDT
	void load_mcfg(const acpi_mcfg *mcfg);

	/// Probe the PCIe busses and add found devices to our
	/// device list. The device list is destroyed and rebuilt.
	void probe_pci();

	lapic *m_lapic;

	static const int max_ioapics = 16;
	ioapic *m_ioapics[16];
	int m_num_ioapics;

	pci_group *m_pci;
	int m_num_pci_groups;

	pci_device *m_devices;
	int m_num_devices;
	int m_num_device_entries;

	device_manager() = delete;
	device_manager(const device_manager &) = delete;
};


/// Information about a discovered PCIe device
class pci_device
{
public:
	/// Default constructor creates an empty object.
	pci_device();

	/// Constructor
	/// \arg group  The group number of this device's bus
	/// \arg bus    The bus number this device is on
	/// \arg device The device number of this device
	/// \arg func   The function number of this device
	pci_device(uint16_t group, uint8_t bus, uint8_t device, uint8_t func);

private:
	uint32_t *m_base;

	/// Bus address: 15:8 bus, 7:3 device, 2:0 device
	uint16_t m_bus_addr;

	uint16_t m_vendor;
	uint16_t m_device;

	uint8_t m_class;
	uint8_t m_subclass;
	uint8_t m_prog_if;
	uint8_t m_revision;

	// Might as well cache these to fill out the struct align
	isr m_irq;
	uint8_t m_header_type;
};
