#include <algorithm>
#include <stdlib.h>
#include <string.h>

#include <util/cdb.h>
#include <zstd.h>

#include "ramdisk.h"

inline constexpr uint64_t manifest_magic = 0x74696e697869736a;  // "jsixinit"
inline constexpr size_t manifest_min = 18;
inline constexpr size_t manifest_version = 1;

using util::read;

ramdisk::ramdisk(util::buffer data) : m_data {data} {}

util::buffer
ramdisk::load_file(const char *name)
{
    util::cdb cdb {m_data};
    util::buffer c = cdb.retrieve(name);
    if (!c.count)
        return c;

    size_t size = ZSTD_getFrameContentSize(c.pointer, c.count); 

    util::buffer d {malloc(size), size};
    size_t out = ZSTD_decompress(
            d.pointer, d.count,
            c.pointer, c.count);

    if (out != size) {
        free(d.pointer);
        return {0,0};
    }

    return d;
}

manifest::manifest(util::buffer data)
{
    if (data.count < manifest_min)
        return;

    char const *base = reinterpret_cast<char const *>(data.pointer);

    if (*read<uint64_t>(data) != manifest_magic)
        return;

    uint8_t version = *read<uint8_t>(data);
    if (version != manifest_version)
        return;

    read<uint8_t>(data); // reserved byte
    uint16_t services_len = *read<uint16_t>(data);
    uint16_t drivers_len = *read<uint16_t>(data);

    base += *read<uint16_t>(data); // start of the string section

}
