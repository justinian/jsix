#pragma once
/// \file device_manager.h
/// The device manager definition
#include "kutil/vector.h"
#include "pci.h"

struct acpi_xsdt;
struct acpi_apic;
struct acpi_mcfg;
class lapic;
class ioapic;


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

	/// Intialize drivers for the current device list.
	void init_drivers();

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
	kutil::vector<ioapic *> m_ioapics;

	kutil::vector<pci_group> m_pci;
	kutil::vector<pci_device> m_devices;

	device_manager() = delete;
	device_manager(const device_manager &) = delete;
};
