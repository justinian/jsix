#include "hardware.h"
#include "console.h"
#include "error.h"
#include "status.h"

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

void
setup_cr4()
{
	uint64_t cr4 = 0;
	asm volatile ( "mov %%cr4, %0" : "=r" (cr4) );
	cr4 |=
		0x000080 | // Enable global pages
		0x000200 | // Enable FXSAVE/FXRSTOR
		0x010000 | // Enable FSGSBASE
		0x020000 | // Enable PCIDs
		0;
	asm volatile ( "mov %0, %%cr4" :: "r" (cr4) );
}

} // namespace hw
} // namespace boot
