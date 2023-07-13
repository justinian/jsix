#include <stddef.h>
#include <j6/memutils.h>
#include <__j6libc/bits.h>
#include <__j6libc/casts.h>
#include "copy.h"

using namespace j6;
using namespace __j6libc;

void *memcpy(void * restrict dst, const void * restrict src, size_t n) {
    asm volatile ("rep movsb"
        :
        : "D"(dst), "S"(src), "c"(n)
        : "memory");
    return dst;
}

static void memmove_dispatch(char *s1, const char *s2, size_t n) {
    if (s1 == s2) return;

    if (s1 < s2 || s1 > s2 + n)
        memcpy(s1, s2, n);
    else
        do_backward_copy(s1, s2, n);
}

void *memmove(void * restrict s1, const void * restrict s2, size_t n) {
    memmove_dispatch(
            reinterpret_cast<char*>(s1),
            reinterpret_cast<const char*>(s1),
            n);

    return s1;
}

#if __UINTPTR_WIDTH__ != 64 && __UINTPTR_WIDTH__ != 32
#error "memset: uintptr_t isn't 4 or 8 bytes"
#endif

static inline uintptr_t repval(uint8_t c) {
    uintptr_t r = c;
    r |= r << 8;
    r |= r << 16;
#if __UINTPTR_WIDTH__ == 64
    r |= r << 32;
#endif
    return r;
}

void *memset(void *s, int c, size_t n) {
    if (!s) return nullptr;

    // First, any unalined initial bytes
    uint8_t *b = cast_to<uint8_t*>(s);
    uint8_t bval = c & 0xff;
    while (n && !is_aligned<uintptr_t>(b)) {
        *b++ = bval;
        --n;
    }

    // As many word-sized writes as possible
    size_t words = n >> word_shift;
    uintptr_t pval = repval(c);
    uintptr_t *p = cast_to<uintptr_t*>(b);
    for (size_t i = 0; i < words; ++i)
        *p++ = pval;

    // Remaining unaligned bytes
    b = reinterpret_cast<uint8_t*>(p);
    size_t remainder = n & word_align_mask;
    for (size_t i = 0; i < remainder; ++i)
        *b++ = bval;

    return s;
}