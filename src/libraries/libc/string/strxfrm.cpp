/** \file strxfrm.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <string.h>

size_t strxfrm(char * restrict s1, const char * restrict s2, size_t n) {
    size_t l = strlen(s2) + 1;
    l = l > n ? n : l;

    memcpy(s1, s2, l);
    s1[l - 1] = 0;

    return l;
}
