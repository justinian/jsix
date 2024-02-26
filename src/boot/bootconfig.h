/// \file bootconfig.h
/// Definitions for reading the jsix bootconfig file
#pragma once

#include <bootproto/bootconfig.h>
#include <util/bitset.h>
#include <util/counted.h>

namespace uefi {
    struct boot_services;
}

namespace boot {

using desc_flags = bootproto::desc_flags;

struct descriptor {
    util::bitset16 flags;
    wchar_t const *path;
};

/// A bootconfig is a manifest of potential files.
class bootconfig
{
public:
    using descriptors = util::counted<descriptor>;

    /// Constructor. Loads bootconfig from the given buffer.
    bootconfig(util::buffer data, uefi::boot_services *bs);

    inline util::bitset16 flags() const { return m_flags; }
    inline const descriptor & kernel() const { return m_kernel; }
    inline const descriptor & init() const { return m_init; }
    inline const wchar_t * initrd() const { return m_initrd; }
    inline const wchar_t * symbols() const { return m_symbols; }
    inline const descriptors & panics() { return m_panics; }

private:
    util::bitset16 m_flags;
    descriptor m_kernel;
    descriptor m_init;
    descriptors m_panics;
    wchar_t const *m_initrd;
    wchar_t const *m_symbols;
};

} // namespace boot
