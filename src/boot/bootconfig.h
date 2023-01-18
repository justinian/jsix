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
    inline const wchar_t * initrd() const { return m_initrd; }
    inline uint16_t initrd_format() const { return m_initrd_format; }
    inline const descriptors & panics() { return m_panics; }

private:
    uint16_t m_flags;
    uint16_t m_initrd_format;
    descriptor m_kernel;
    descriptor m_init;
    descriptors m_panics;
    wchar_t const *m_initrd;
};

} // namespace boot
