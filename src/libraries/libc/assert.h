#pragma once
/** \file assert.h
  * Diagnostics
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <stdnoreturn.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#define assert( argument ) ( (void) 0 )
#else

_Noreturn void __assert_fail( const char *, const char *, unsigned, const char * );

#define assert( argument ) \
    do { if (!(argument)) { __assert_fail( #argument, __FILE__, __LINE__, __func__ ); }} while(0)

#endif

#ifndef __cplusplus
#define static_assert _Static_assert
#endif

#ifdef __cplusplus
} // extern "C"
#endif
