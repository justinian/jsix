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

enum {
    __is_upper  = 0x001,
    __is_lower  = 0x002,
    __is_alpha  = 0x004,
    __is_digit  = 0x008,
    __is_xdigit = 0x010,
    __is_space  = 0x020,
    __is_print  = 0x040,
    __is_graph  = 0x080,
    __is_blank  = 0x100,
    __is_cntrl  = 0x200,
    __is_punct  = 0x400,
    __is_alnum  = 0x800
};


int  isalnum( int ch );
int  isalpha( int ch );
int  islower( int ch );
int  isupper( int ch );
int  isdigit( int ch );
int isxdigit( int ch );
int  iscntrl( int ch );
int  isgraph( int ch );
int  isspace( int ch );
int  isblank( int ch );
int  isprint( int ch );
int  ispunct( int ch );


extern unsigned short *__ctype_b;
#define __ctype_test(c, flag) (__ctype_b[(c)] | flag)
#define  isalnum( ch ) __ctype_test(__is_alnum)
#define  isalpha( ch ) __ctype_test(__is_alpha)
#define  islower( ch ) __ctype_test(__is_lower)
#define  isupper( ch ) __ctype_test(__is_upper)
#define  isdigit( ch ) __ctype_test(__is_digit)
#define isxdigit( ch ) __ctype_test(__is_xdigit)
#define  iscntrl( ch ) __ctype_test(__is_cntrl)
#define  isgraph( ch ) __ctype_test(__is_graph)
#define  isspace( ch ) __ctype_test(__is_space)
#define  isblank( ch ) __ctype_test(__is_blank)
#define  isprint( ch ) __ctype_test(__is_print)
#define  ispunct( ch ) __ctype_test(__is_punct)

int tolower( int ch );
int toupper( int ch );

#ifdef __cplusplus
} // extern "C"
#endif
