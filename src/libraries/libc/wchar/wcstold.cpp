/** \file wcstold.cpp
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

long double wcstold(const wchar_t * restrict nptr, wchar_t ** restrict endptr)
{
    assert(!"wcstold() NYI");
    return 0;
}
