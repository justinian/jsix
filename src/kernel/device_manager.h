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

using irq_callback = void (*)(void *);


/// Manager for all system hardware devices
class device_manager
{
public:
	/// Constructor.
	/// \arg root_table   Pointer to the ACPI RSDP
	device_manager(const void *root_table);

	/// Get the system global device manager.
	/// \returns  A reference to the system device manager
	static device_manager & get() { return s_instance; }

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

	/// Allocate an MSI IRQ for a device
	/// \arg name    Name of the interrupt, for display to user
	/// \arg device  Device this MSI is being allocated for
	/// \arg cb      Callback to call when the interrupt is received
	/// \arg data    Data to pass to the callback
	/// \returns     True if an interrupt was allocated successfully
	bool allocate_msi(
			const char *name,
			pci_device &device,
			irq_callback cb,
			void *data);

	/// Dispatch an IRQ interrupt
	/// \arg irq  The irq number of the interrupt
	/// \returns  True if the interrupt was handled
	inline bool dispatch_irq(uint8_t irq)
	{
		if (irq < m_irqs.count()) {
			irq_allocation &cba = m_irqs[irq];
			if (cba.callback) {
				cba.callback(cba.data);
				return true;
			}
		}
		return false;
	}

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

	/// Handle a bad IRQ. Called when an interrupt is dispatched
	/// that has no callback.
	void bad_irq(uint8_t irq);

	lapic *m_lapic;
	kutil::vector<ioapic *> m_ioapics;

	kutil::vector<pci_group> m_pci;
	kutil::vector<pci_device> m_devices;

	struct irq_allocation
	{
		const char *name;
		irq_callback callback;
		void *data;
	};
	kutil::vector<irq_allocation> m_irqs;

	static device_manager s_instance;

	device_manager() = delete;
	device_manager(const device_manager &) = delete;
	device_manager operator=(const device_manager &) = delete;
};
