#pragma once
/// \file bootproto/bootconfig.h
/// Data structures for reading jsix_boot.dat

#include <stdint.h>
#include <util/enum_bitfields.h>

namespace bootproto {

enum class desc_flags : uint16_t {
    graphical = 0x01,
    panic     = 0x02,
    symbols   = 0x04,
};
is_bitfield(desc_flags);

} // namespace bootproto
