#pragma once

namespace util {

/// Reverse the order of bytes in a 32 bit integer
constexpr uint32_t
byteswap32(uint32_t x) {
    return ((x >> 24) & 0x000000ff) | ((x >>  8) & 0x0000ff00)
        | ((x <<  8) & 0x00ff0000) | ((x << 24) & 0xff000000);
}

/// Do a simple byte-wise checksum of an area of memory. The area
/// summed will be the bytes at indicies [off, len).
/// \arg p    The start of the memory region
/// \arg len  The number of bytes in the region
/// \arg off  An optional offset into the region
uint8_t checksum(const void *p, size_t len, size_t off = 0) {
    uint8_t sum = 0;
    const uint8_t *c = reinterpret_cast<const uint8_t *>(p);
    for (size_t i = off; i < len; ++i) sum += c[i];
    return sum;
}

}
