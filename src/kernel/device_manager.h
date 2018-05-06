#pragma once

struct acpi_xsdt;
struct acpi_apic;
struct acpi_mcfg;

class lapic;
class ioapic;
struct pci_group;

class device_manager
{
public:
	device_manager(const void *root_table);

	device_manager() = delete;
	device_manager(const device_manager &) = delete;

	lapic * get_lapic() { return m_lapic; }
	ioapic * get_ioapic(int i);

private:
	lapic *m_lapic;

	static const int max_ioapics = 16;
	ioapic *m_ioapics[16];
	int m_num_ioapics;

	pci_group *m_pci;
	int m_num_pci_groups;

	void load_xsdt(const acpi_xsdt *xsdt);
	void load_apic(const acpi_apic *apic);
	void load_mcfg(const acpi_mcfg *mcfg);
};
