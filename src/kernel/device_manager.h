#pragma once

struct acpi_xsdt;
struct acpi_apic;

class lapic;
class ioapic;

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
	ioapic *m_ioapics;
	int m_num_ioapics;

	void load_xsdt(const acpi_xsdt *xsdt);
	void load_apic(const acpi_apic *apic);
};
