#pragma once
/// \file bootproto/init.h
/// Data structures for initializing the init server

#include <stddef.h>
#include <stdint.h>

#include <util/counted.h>
#include <util/enum_bitfields.h>

namespace bootproto {

enum class module_type : uint8_t { none, initrd, device, };

struct module
{
    uint16_t bytes;     //< Size of this module in bytes, including this header
    module_type type;   //< Type of module
    // 5 bytes padding
    uint64_t type_id;   //< Optional subtype or id

    template <typename T> T * data() { return reinterpret_cast<T*>(this+1); }
    template <typename T> const T * data() const { return reinterpret_cast<const T*>(this+1); }
};

struct modules_page
{
    modules_page *next;
};

} // namespace bootproto
