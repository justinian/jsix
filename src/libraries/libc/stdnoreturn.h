#pragma once
/** \file stdnoreturn.h
  * noreturn convenience macros
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#if !defined(__cplusplus) && __STDC_VERSION__ >= 201103L
#define noreturn _Noreturn
#elif !defined(__cplusplus)
#define noreturn
#endif
