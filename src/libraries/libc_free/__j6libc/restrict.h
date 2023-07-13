#pragma once
/** \file j6libc/restrict.h
  * Internal handling of restrict keyword for C++
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#ifdef __cplusplus
#define restrict __restrict__
#endif
