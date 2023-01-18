#pragma once
/// \file bootproto/init.h
/// Data structures for initializing the init server

#include <stddef.h>
#include <stdint.h>

#include <util/counted.h>
#include <util/enum_bitfields.h>

namespace bootproto {

enum class module_type : uint8_t { none, initrd, device, };
enum class initrd_format : uint8_t { none, zstd, };
enum class device_type : uint16_t { none, uefi_fb, };

struct module
{
    module_type type;
    // 1 byte padding
    uint16_t subtype;
    // 4 bytes padding
    util::buffer data;
};

struct module_page_header
{
    uint8_t count;
    uintptr_t next;
};

struct modules_page :
    public module_page_header
{
    static constexpr unsigned per_page = (0x1000 - sizeof(module_page_header)) / sizeof(module);
    module modules[per_page];
};

} // namespace bootproto
