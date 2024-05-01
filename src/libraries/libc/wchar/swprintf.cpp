/** \file swprintf.cpp
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <assert.h>
#include <wchar.h>

int swprintf(wchar_t * restrict s, size_t n, const wchar_t * restrict format, ...)
{
    assert(!"swprintf() NYI");
    return 0;
}
