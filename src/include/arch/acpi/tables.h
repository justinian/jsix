#pragma once

#include <stddef.h>
#include <stdint.h>

#include <util/enum_bitfields.h>
#include <util/misc.h> // for byteswap32

namespace acpi {

struct table_header
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

    bool validate() const { return util::checksum(this, length) == 0; }
} __attribute__ ((packed));


#define TABLE_HEADER(signature) \
    static constexpr uint32_t type_id = util::byteswap32(signature); \
    table_header header;

template <typename T>
bool validate(const T *t) {
    return t->header.validate(T::type_id);
}

template <typename T>
size_t table_entries(const T *t, size_t size) {
    return (t->header.length - sizeof(T)) / size;
}


enum class gas_type : uint8_t
{
    system_memory,
    system_io,
    pci_config,
    embedded,
    smbus,
    platform_channel    = 0x0a,
    functional_fixed    = 0x7f
};

struct gas
{
    gas_type type;

    uint8_t reg_bits;
    uint8_t reg_offset;
    uint8_t access_size;

    uint64_t address;
} __attribute__ ((packed));


enum class fadt_flags : uint32_t
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
is_bitfield(fadt_flags);

struct fadt
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
    fadt_flags flags;

    gas reset_reg;
    uint8_t reset_value;

    uint16_t arm_boot_arch;

    uint8_t fadt_minor_version;

    uint64_t x_facs;
    uint64_t x_dsdt;

    gas x_pm1a_event_block;
    gas x_pm1b_event_block;
    gas x_pm1a_control_block;
    gas x_pm1b_control_block;
    gas x_pm2_control_block;
    gas x_pm_timer_block;
    gas x_gpe0_block;
    gas x_gpe1_block;

    gas sleep_control_reg;
    gas sleep_status_reg;

    uint64_t hypervisor_vendor_id;
} __attribute__ ((packed));

struct xsdt
{
    TABLE_HEADER('XSDT');
    table_header *headers[0];
} __attribute__ ((packed));

struct apic
{
    TABLE_HEADER('APIC');
    uint32_t local_address;
    uint32_t flags;
    uint8_t controller_data[0];
} __attribute__ ((packed));

struct mcfg_entry
{
    uint64_t base;
    uint16_t group;
    uint8_t bus_start;
    uint8_t bus_end;
    uint32_t reserved;
} __attribute__ ((packed));

struct mcfg
{
    TABLE_HEADER('MCFG');
    uint64_t reserved;
    mcfg_entry entries[0];
} __attribute__ ((packed));

struct hpet
{
    TABLE_HEADER('HPET');
    uint32_t hardware_id;
    gas base_address;
    uint8_t index;
    uint16_t periodic_min;
    uint8_t attributes;
} __attribute__ ((packed));

struct bgrt
{
    TABLE_HEADER('BGRT');
    uint16_t version;
    uint8_t status;
    uint8_t type;
    uintptr_t address;
    uint32_t offset_x;
    uint32_t offset_y;
} __attribute__ ((packed));

struct rsdp1
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__ ((packed));

struct rsdp2
{
    char signature[8];
    uint8_t checksum10;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;

    uint32_t length;
    table_header *xsdt_address;
    uint8_t checksum20;
    uint8_t reserved[3];
} __attribute__ ((packed));

template <typename T> static const T *
check_get_table(const table_header *header)
{
    if (!header || header->type != T::type_id)
        return nullptr;
    return reinterpret_cast<const T *>(header);
}

} // namespace acpi
