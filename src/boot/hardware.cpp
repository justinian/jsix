#include "hardware.h"
#include "console.h"
#include "error.h"

namespace boot {
namespace hw {

void *
find_acpi_table(uefi::system_table *st)
{
	status_line status(L"Searching for ACPI table");

	// Find ACPI tables. Ignore ACPI 1.0 if a 2.0 table is found.
	uintptr_t acpi1_table = 0;

	for (size_t i = 0; i < st->number_of_table_entries; ++i) {
		uefi::configuration_table *table = &st->configuration_table[i];

		// If we find an ACPI 2.0 table, return it immediately
		if (table->vendor_guid == uefi::vendor_guids::acpi2)
			return table->vendor_table;

		if (table->vendor_guid == uefi::vendor_guids::acpi1) {
			// Mark a v1 table with the LSB high
			acpi1_table = reinterpret_cast<uintptr_t>(table->vendor_table);
			acpi1_table |= 1;
		}
	}

	if (!acpi1_table) {
		error::raise(uefi::status::not_found, L"Could not find ACPI table");
	} else if (acpi1_table & 1) {
		status_line::warn(L"Only found ACPI 1.0 table");
	}

	return reinterpret_cast<void*>(acpi1_table);
}


} // namespace hw
} // namespace boot
