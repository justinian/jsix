#pragma once

#include <uefi/tables.h>

namespace boot {
namespace hw {

/// Find the ACPI table in the system configuration tables
/// and return a pointer to it. If only an ACPI 1.0 table is
/// available, the returned pointer will have its least
/// significant bit set to 1.
void * find_acpi_table(uefi::system_table *st);

} // namespace hw
} // namespace boot
