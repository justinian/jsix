/** \file memmove.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <string.h>
#include <__j6libc/copy.h>

using namespace __j6libc;

void memmove_dispatch(char *s1, const char *s2, size_t n) {
    if (s1 == s2) return;

    if (s1 < s2 || s1 > s2 + n)
        memcpy(s1, s2, n);
    else
        do_backward_copy(s1, s2, n);
}

void *memmove(void * restrict s1, const void * restrict s2, size_t n) {
    memmove_dispatch(
            reinterpret_cast<char*>(s1),
            reinterpret_cast<const char*>(s2),
            n);

    return s1;
}

