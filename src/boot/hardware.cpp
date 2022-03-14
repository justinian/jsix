#include <cpu/cpu_id.h>

#include "console.h"
#include "error.h"
#include "hardware.h"
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

static uint64_t
rdmsr(uint32_t addr)
{
    uint32_t low, high;
    __asm__ __volatile__ ("rdmsr" : "=a"(low), "=d"(high) : "c"(addr));
    return (static_cast<uint64_t>(high) << 32) | low;
}

static void
wrmsr(uint32_t addr, uint64_t value)
{
    uint32_t low = value & 0xffffffff;
    uint32_t high = value >> 32;
    __asm__ __volatile__ ("wrmsr" :: "c"(addr), "a"(low), "d"(high));
}

void
setup_control_regs()
{
    uint64_t cr4 = 0;
    asm volatile ( "mov %%cr4, %0" : "=r" (cr4) );
    cr4 |= 0x000080; // Enable global pages
    asm volatile ( "mov %0, %%cr4" :: "r" (cr4) );

    // Set up IA32_EFER
    constexpr uint32_t IA32_EFER = 0xC0000080;
    uint64_t efer = rdmsr(IA32_EFER);
    efer |=
        0x0001 | // Enable SYSCALL
        0x0800 | // Enable NX bit
        0;
    wrmsr(IA32_EFER, efer);
}

void
check_cpu_supported()
{
    status_line status {L"Checking CPU features"};

    cpu::cpu_id cpu;
    cpu::cpu_id::features features = cpu.validate();
    bool supported = true;

#define CPU_FEATURE_OPT(...)
#define CPU_FEATURE_REQ(name, ...) \
    if (!features[cpu::feature::name]) { \
        status::fail(L"CPU required feature " L ## #name, uefi::status::unsupported); \
        supported = false; \
    }
#include "cpu/features.inc"
#undef CPU_FEATURE_REQ
#undef CPU_FEATURE_OPT

    if (!supported)
        error::raise(uefi::status::unsupported, L"CPU not supported");
}


} // namespace hw
} // namespace boot
