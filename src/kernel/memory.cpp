#include "memory.h"

namespace std {
    enum class __attribute__ ((__type_visibility("default"))) align_val_t : size_t { };
}

// Implementation of memset and memcpy because we're not
// linking libc into the kernel
extern "C" {

void *
memset(void *s, uint8_t v, size_t n)
{
    uint8_t *p = reinterpret_cast<uint8_t *>(s);
    for (size_t i = 0; i < n; ++i) p[i] = v;
    return s;
}

void *
memcpy(void *dest, const void *src, size_t n)
{
    const uint8_t *s = reinterpret_cast<const uint8_t *>(src);
    uint8_t *d = reinterpret_cast<uint8_t *>(dest);
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return d;
}

}

uint8_t
checksum(const void *p, size_t len, size_t off)
{
    uint8_t sum = 0;
    const uint8_t *c = reinterpret_cast<const uint8_t *>(p);
    for (int i = off; i < len; ++i) sum += c[i];
    return sum;
}
