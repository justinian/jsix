#pragma once
/** \file stdarg.h
  * Variable arguments
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

typedef __builtin_va_list va_list;

#define va_arg( ap, type )    (__builtin_va_arg( (ap), type ))
#define va_copy( dest, src )  (__builtin_va_copy( (dest), (src) ))
#define va_end( ap )          (__builtin_va_end( (ap) ))
#define va_start( ap, parmN ) (__builtin_va_start( (ap), (parmN) ))
