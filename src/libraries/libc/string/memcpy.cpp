/** \file memcpy.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <string.h>
#include <stddef.h>
#include <__j6libc/copy.h>

using namespace __j6libc;

inline void memcpy_dispatch(char *s1, const char *s2, size_t n) {
    if (n ==  0) return;
    if (n ==  1) return do_copy<1>(s1, s2);
    if (n ==  2) return do_copy<2>(s1, s2);
    if (n ==  3) return do_copy<3>(s1, s2);
    if (n ==  4) return do_copy<4>(s1, s2);
    if (n  <  8) return do_double_copy<4>(s1, s2, n);
    if (n ==  8) return do_copy<8>(s1, s2);
    if (n  < 16) return do_double_copy<8>(s1, s2, n);
    if (n  < 32) return do_double_copy<16>(s1, s2, n);
    if (n  < 64) return do_double_copy<32>(s1, s2, n);

    if (n  < 128)
        return do_double_copy<64>(s1, s2, n);
    else
        return do_large_copy(s1, s2, n);
}

void *memcpy(void * restrict s1, const void * restrict s2, size_t n) {
    memcpy_dispatch(
            reinterpret_cast<char*>(s1),
            reinterpret_cast<const char*>(s2),
            n);

    return s1;
}
