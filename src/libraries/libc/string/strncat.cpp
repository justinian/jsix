/** \file strncat.cpp
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

char *strncat(char * restrict s1, const char * restrict s2, size_t n) {
    if (!s1 || !s2 || !n)
        return s1;

    char *dest = s1;
    char const *src = s2;
    while (*dest++);
    while (*src && n--) *dest++ = *src++;
    *dest = 0;

    return s1;
}

