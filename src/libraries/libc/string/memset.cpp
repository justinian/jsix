/** \file memset.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <string.h>
#include <__j6libc/bits.h>
#include <__j6libc/casts.h>

using namespace __j6libc;

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
