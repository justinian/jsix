#pragma once
/** \file j6libc/wchar_t.h
  * Internal definition of wchar_t
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#ifndef __cplusplus
typedef __WCHAR_TYPE__   wchar_t;
#endif

#define WCHAR_MAX  __WCHAR_MAX__
#define WCHAR_MIN  ((-__WCHAR_MAX__) - 1)
