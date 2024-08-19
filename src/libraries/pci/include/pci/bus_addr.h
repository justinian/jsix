#pragma once
/// \file bus_addr.h
/// PCIe Bus Address structure

#include <stdint.h>

namespace pci {

/// Bus address: 15:8 bus, 7:3 device, 2:0 device
struct bus_addr
{
    static constexpr uint16_t max_functions =  8;
    static constexpr uint16_t max_devices   = 32;

    uint16_t function : 3;
    uint16_t device   : 5;
    uint16_t bus      : 8;

    uint16_t as_int() const { return *reinterpret_cast<const uint16_t*>(this); }
    operator uint16_t() const { return as_int(); }
};

inline bool operator==(const bus_addr &a, const bus_addr &b) { return a.as_int() == b.as_int(); }
inline bool operator!=(const bus_addr &a, const bus_addr &b) { return a.as_int() != b.as_int(); }
inline bool operator< (const bus_addr &a, const bus_addr &b) { return a.as_int() < b.as_int(); }
inline bool operator> (const bus_addr &a, const bus_addr &b) { return a.as_int() > b.as_int(); }

} // namespace pci
