/** \file memcmp.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n) {
    if (!s1 || !s2) return 0;

    char const * c1 = reinterpret_cast<char const*>(s1);
    char const * c2 = reinterpret_cast<char const*>(s2);

    while (n && *c1++ == *c2++) n--;
    if (!n) return 0;

    if (c1 > c2) return 1;
    return -1;
}

extern "C" int bcmp(const void *s1, const void *s2, size_t n)
    __attribute__ ((weak, alias ("memcmp")));
