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

void *memcpy(void * restrict s1, const void * restrict s2, size_t n) {
    asm volatile ("rep movsb" : "+D"(s1), "+S"(s2), "+c"(n) :: "memory");
    return s1;
}
