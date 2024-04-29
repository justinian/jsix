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

int strncmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2) return 0;

    char const * c1 = s1;
    char const * c2 = s2;

    while (n && *c1 && *c2 && *c1 == *c2) {
        n--; c1++; c2++;
    }

    if (!n || *c1 == *c2) return 0;
    if (!*c2 || *c1 > *c2) return 1;
    return -1;
}

