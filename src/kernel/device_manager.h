#pragma once

struct acpi_xsdt;
struct acpi_apic;

class device_manager
{
public:
	device_manager(const void *root_table);

	device_manager() = delete;
	device_manager(const device_manager &) = delete;

	uint8_t * local_apic() const;
	uint8_t * io_apic() const;

private:
	uint32_t *m_local_apic;
	uint32_t *m_io_apic;

	uint32_t m_global_interrupt_base;

	void load_xsdt(const acpi_xsdt *xsdt);
	void load_apic(const acpi_apic *apic);
};
