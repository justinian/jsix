#pragma once
/// \file device_manager.h
/// The device manager definition
#include "kutil/vector.h"
#include "apic.h"
#include "hpet.h"
#include "pci.h"

struct acpi_xsdt;
struct acpi_apic;
struct acpi_mcfg;
struct acpi_hpet;
class block_device;

using irq_callback = void (*)(void *);


/// Manager for all system hardware devices
class device_manager
{
public:
	/// Constructor.
	device_manager();

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

	/// Parse ACPI tables.
	/// \arg root_table  Pointer to the ACPI RSDP
	void parse_acpi(const void *root_table);

	/// Intialize drivers for the current device list.
	void init_drivers();

	/// Install an IRQ callback for a device
	/// \arg irq   IRQ to install the handler for
	/// \arg name  Name of the interrupt, for display to user
	/// \arg cb    Callback to call when the interrupt is received
	/// \arg data  Data to pass to the callback
	/// \returns   True if an IRQ was installed successfully
	bool install_irq(
			unsigned irq,
			const char *name,
			irq_callback cb,
			void *data);

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

	/// Register the existance of a block device.
	/// \arg blockdev  Pointer to the block device
	void register_block_device(block_device *blockdev);

	/// Get the number of block devices in the system
	/// \returns  A count of devices
	inline unsigned get_num_block_devices() const { return m_blockdevs.count(); }

	/// Get a block device
	/// \arg i    Index of the device to get
	/// \returns  A pointer to the requested device, or nullptr
	inline block_device * get_block_device(unsigned i)
	{
		return i < m_blockdevs.count() ?
			m_blockdevs[i] : nullptr;
	}

	/// Get an HPET device
	/// \arg i    Index of the device to get
	/// \returns  A pointer to the requested device, or nullptr
	inline hpet * get_hpet(unsigned i)
	{
		return i < m_hpets.count() ?
			&m_hpets[i] : nullptr;
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

	/// Parse the ACPI HPET and initialize an HPET from it.
	/// \arg hpet  Pointer to the HPET from the XSDT
	void load_hpet(const acpi_hpet *hpet);

	/// Probe the PCIe busses and add found devices to our
	/// device list. The device list is destroyed and rebuilt.
	void probe_pci();

	/// Handle a bad IRQ. Called when an interrupt is dispatched
	/// that has no callback.
	void bad_irq(uint8_t irq);

	lapic *m_lapic;
	kutil::vector<ioapic> m_ioapics;
	kutil::vector<hpet> m_hpets;

	kutil::vector<pci_group> m_pci;
	kutil::vector<pci_device> m_devices;

	struct irq_allocation
	{
		const char *name = nullptr;
		irq_callback callback = nullptr;
		void *data = nullptr;
	};
	kutil::vector<irq_allocation> m_irqs;

	kutil::vector<block_device *> m_blockdevs;

	static device_manager s_instance;

	device_manager(const device_manager &) = delete;
	device_manager operator=(const device_manager &) = delete;
};
