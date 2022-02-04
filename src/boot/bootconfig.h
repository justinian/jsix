/// \file bootconfig.h
/// Definitions for reading the jsix bootconfig file
#pragma once

#include <bootproto/bootconfig.h>
#include <util/counted.h>

namespace uefi {
    struct boot_services;
}

namespace boot {

using desc_flags = bootproto::desc_flags;

struct descriptor {
    desc_flags flags;
    wchar_t const *path;
    wchar_t const *desc;
};

/// A bootconfig is a manifest of potential files.
class bootconfig
{
public:
    using descriptors = util::counted<descriptor>;

    /// Constructor. Loads bootconfig from the given buffer.
    bootconfig(util::buffer data, uefi::boot_services *bs);

    inline uint16_t flags() const { return m_flags; }
    inline const descriptor & kernel() const { return m_kernel; }
    inline const descriptor & init() const { return m_init; }
    descriptors programs() { return m_programs; }
    descriptors data() { return m_data; }

private:
    uint16_t m_flags;
    descriptor m_kernel;
    descriptor m_init;
    descriptors m_programs;
    descriptors m_data;
};

} // namespace boot
