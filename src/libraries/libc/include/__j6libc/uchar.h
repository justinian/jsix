#pragma once
/** \file j6libc/uchar.h
  * Internal definition of char16_t and char32_t
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

typedef __UINT_LEAST16_TYPE__  char16_t;
typedef __UINT_LEAST32_TYPE__  char32_t;
