#pragma once
/** \file ctype.h
  * Character handling
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#ifdef __cplusplus
extern "C" {
#endif

int isalnum( int ch );
int isalpha( int ch );
int islower( int ch );
int isupper( int ch );
int isdigit( int ch );
int isxdigit( int ch );
int iscntrl( int ch );
int isgraph( int ch );
int isspace( int ch );
int isblank( int ch );
int isprint( int ch );
int ispunct( int ch );

int tolower( int ch );
int toupper( int ch );

#ifdef __cplusplus
} // extern "C"
#endif
