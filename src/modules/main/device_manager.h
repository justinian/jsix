#pragma once

struct acpi_xsdt;

class device_manager
{
public:
	device_manager(const void *root_table);

	device_manager() = delete;
	device_manager(const device_manager &) = delete;

private:
	void load_xsdt(const acpi_xsdt *xsdt);
};
