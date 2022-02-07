/** \file strcspn.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <string.h>

size_t strcspn(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;

    char const *p = s1;
    size_t s2len = strlen(s2);
    size_t total = 0;
    while (char c = *p) {
        for (size_t i = 0; i < s2len; ++i)
            if (c == s2[i]) return total;
        ++total;
    }
    return total;
}
