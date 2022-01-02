#pragma once

#include <stddef.h>
#include <stdint.h>

#include "enum_bitfields.h"
#include "kutil/misc.h"

struct acpi_table_header
{
    uint32_t type;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;

    bool validate(uint32_t expected_type = 0) const;
} __attribute__ ((packed));

#define TABLE_HEADER(signature) \
    static constexpr uint32_t type_id = kutil::byteswap(signature); \
    acpi_table_header header;


template <typename T>
bool acpi_validate(const T *t) { return t->header.validate(T::type_id); }

template <typename T>
size_t acpi_table_entries(const T *t, size_t size)
{
    return (t->header.length - sizeof(T)) / size;
}

enum class acpi_gas_type : uint8_t
{
    system_memory,
    system_io,
    pci_config,
    embedded,
    smbus,
    platform_channel    = 0x0a,
    functional_fixed    = 0x7f
};

struct acpi_gas
{
    acpi_gas_type type;

    uint8_t reg_bits;
    uint8_t reg_offset;
    uint8_t access_size;

    uint64_t address;
} __attribute__ ((packed));


enum class acpi_fadt_flags : uint32_t
{
    wbinvd          = 0x00000001,
    wbinvd_flush    = 0x00000002,
    proc_c1         = 0x00000004,
    p_lvl2_up       = 0x00000008,
    pwr_button      = 0x00000010,
    slp_button      = 0x00000020,
    fix_rtc         = 0x00000040,
    rtc_s4          = 0x00000080,
    tmr_val_ext     = 0x00000100,
    dck_cap         = 0x00000200,
    reset_reg_sup   = 0x00000400,
    sealed_case     = 0x00000800,
    headless        = 0x00001000,
    cpu_sw_slp      = 0x00002000,
    pci_exp_wak     = 0x00004000,
    use_plat_clock  = 0x00008000,
    s4_rtc_sts_val  = 0x00010000,
    remote_pwr_cap  = 0x00020000,
    apic_cluster    = 0x00040000,
    apic_physical   = 0x00080000,
    hw_reduced_acpi = 0x00100000,
    low_pwr_s0_idle = 0x00200000
};
is_bitfield(acpi_fadt_flags);

struct acpi_fadt
{
    TABLE_HEADER('FACP');

    uint32_t facs;
    uint32_t dsdt;

    uint8_t reserved0;

    uint8_t preferred_pm_profile;

    uint16_t sci_interrupt;
    uint32_t smi_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_block_length;
    uint8_t gpe1_block_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint16_t cstate2_latency;
    uint16_t cstate3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    uint16_t iapc_boot_arch;
    uint8_t reserved1;
    acpi_fadt_flags flags;

    acpi_gas reset_reg;
    uint8_t reset_value;

    uint16_t arm_boot_arch;

    uint8_t fadt_minor_version;

    uint64_t x_facs;
    uint64_t x_dsdt;

    acpi_gas x_pm1a_event_block;
    acpi_gas x_pm1b_event_block;
    acpi_gas x_pm1a_control_block;
    acpi_gas x_pm1b_control_block;
    acpi_gas x_pm2_control_block;
    acpi_gas x_pm_timer_block;
    acpi_gas x_gpe0_block;
    acpi_gas x_gpe1_block;

    acpi_gas sleep_control_reg;
    acpi_gas sleep_status_reg;

    uint64_t hypervisor_vendor_id;
} __attribute__ ((packed));

struct acpi_xsdt
{
    TABLE_HEADER('XSDT');
    acpi_table_header *headers[0];
} __attribute__ ((packed));

struct acpi_apic
{
    TABLE_HEADER('APIC');
    uint32_t local_address;
    uint32_t flags;
    uint8_t controller_data[0];
} __attribute__ ((packed));

struct acpi_mcfg_entry
{
    uint64_t base;
    uint16_t group;
    uint8_t bus_start;
    uint8_t bus_end;
    uint32_t reserved;
} __attribute__ ((packed));

struct acpi_mcfg
{
    TABLE_HEADER('MCFG');
    uint64_t reserved;
    acpi_mcfg_entry entries[0];
} __attribute__ ((packed));

struct acpi_hpet
{
    TABLE_HEADER('HPET');
    uint32_t hardware_id;
    acpi_gas base_address;
    uint8_t index;
    uint16_t periodic_min;
    uint8_t attributes;
} __attribute__ ((packed));

struct acpi_bgrt
{
    TABLE_HEADER('BGRT');
    uint16_t version;
    uint8_t status;
    uint8_t type;
    uintptr_t address;
    uint32_t offset_x;
    uint32_t offset_y;
} __attribute__ ((packed));

