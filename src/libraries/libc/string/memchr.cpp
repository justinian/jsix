/** \file memchr.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <string.h>
#include <__j6libc/casts.h>

using namespace __j6libc;

void *memchr(const void *s, int c, size_t n) {
    if (!s || !n) return nullptr;

    char const *b = reinterpret_cast<char const*>(s);
    while(n && *b && *b != c) { b++; n--; }

    return (n && *b) ? cast_to<void*>(b) : nullptr;
}
