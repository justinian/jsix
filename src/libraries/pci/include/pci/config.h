#pragma once
/// \file config.h
/// PCIe device configuration headers

#include <stdint.h>
#include <util/bitset.h>

namespace pci {

/// Command register bits
enum class control 
{
    io_space,    // Respond to IO space access
    mem_space,   // Respond to memory space access
    bus_master,  // Allow acting as a bus master
    special,     // (Not in PCIe) Enable Special Cycle
    mwi_enable,  // (Not in PCIe) Enable Memory Write and Invalidate
    vga_snoop,   // (Not in PCIe) Enable VGA Palette Snoop
    parity_err,  // Enable PERR# assertion for parity errors
    reserved7,
    serr_enable, // Enable SERR# pin
    fb2b_enable, // (Not in PCIe) Allow fast back-to-back
    int_disable, // Disable INTx# assertions
};

/// Status register bits
enum class status
{
    intr        =  3, // Interrupt status
    caps        =  4, // Has capability list
    mhz66       =  5, // (Not in PCIe) Can run at 66MHz
    fb2b        =  7, // (Not in PCIe) Supports fast back-to-back
    md_parity   =  8, // Master data parity error
    tgt_abort_s = 11, // Target-Abort sent
    tgt_abort_r = 12, // Target-Abort received
    mst_abort_r = 13, // Master-Abort received
    sys_err_s   = 14, // SERR# asserted
    parity      = 15, // Parity error dedected
};

struct header_base
{
    uint16_t vendor;                // Vendor ID
    uint16_t device;                // Vendor's device ID

    util::bitset16 command;
    util::bitset16 status;

    uint8_t revision;               // Device revision
    uint8_t device_class[3];

    uint8_t cache_line;             // Not used in PCIe
    uint8_t master_latency;         // Not used in PCIe
    uint8_t header_type : 7;
    uint8_t multi_device : 1;
    uint8_t bist;                   // Built-in Self Test
};

struct header_unknown :
    public header_base
{
    uint8_t unknown1[36];

    uint8_t caps;
    uint8_t unknown2[7];

    uint8_t int_line;
    uint8_t int_pin;

    uint8_t unknown3[2];
};

/// Device configuration header
struct header0 :
    public header_base
{
    uint32_t bar[6];                        // Base address registers

    uint32_t cis;

    uint16_t subsystem_vendor;
    uint16_t subsystem;

    uint32_t exrom;

    uint8_t caps;
    uint8_t reserved1[3];
    uint32_t reserved2;

    uint8_t int_line;
    uint8_t int_pin;
    uint8_t min_gnt;                        // Not used in PCIe
    uint8_t max_lat;                        // Not used in PCIe
};

/// Bridge configuration header
struct header1 :
    public header_base
{
    uint32_t bar[2];                        // Base address registers

    uint8_t pri_bus;                        // Not used in PCIe
    uint8_t sec_bus;
    uint8_t sub_bus;
    uint8_t sec_lat;                        // Not used in PCIe

    uint8_t io_base;
    uint8_t io_limit;
    uint16_t sec_status;

    uint16_t mem_base;
    uint16_t mem_limit;
    uint16_t prefetch_mem_base;
    uint16_t prefetch_mem_limit;

    uint32_t prefetch_mem_base_high;
    uint32_t prefetch_mem_limit_high;
    uint16_t io_base_high;
    uint16_t io_limit_high;

    uint8_t caps;
    uint8_t reserved1[3];

    uint32_t exrom;

    uint8_t int_line;
    uint8_t int_pin;
    uint16_t bridge_control;
};

} // namespace pci
