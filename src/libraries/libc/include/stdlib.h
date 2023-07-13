#pragma once
/** \file stdlib.h
  * General utilities
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <__j6libc/null.h>
#include <__j6libc/restrict.h>
#include <__j6libc/size_t.h>
#include <__j6libc/wchar_t.h>

#ifdef __cplusplus
extern "C" {
#endif

// Numeric conversion functions
//
double atof( const char *nptr );
int atoi( const char *nptr );
long atol( const char *nptr );
long long atoll( const char *nptr );

double strtod( const char * restrict nptr, char ** restrict endptr );
float strtof( const char * restrict nptr, char ** restrict endptr );
long double strtold( const char * restrict nptr, char ** restrict endptr );

long strtol( const char * restrict nptr, char ** restrict endptr, int base );
long long strtoll( const char * restrict nptr, char ** restrict endptr, int base );
unsigned long strtoul( const char * restrict nptr, char ** restrict endptr, int base );
unsigned long long strtoull( const char * restrict nptr, char ** restrict endptr, int base );

// Pseudo-random sequence generation functions
//
#define RAND_MAX 0

int rand( void );
void srand( unsigned int seed );

// Memory management functions
//
void *aligned_alloc( size_t alignment, size_t size );
void *calloc( size_t nmemb, size_t size);
void free( void *ptr );
void *malloc( size_t size );
void *realloc( void *ptr, size_t size );

// Bonus functions from dlmalloc
void* realloc_in_place( void *ptr, size_t size );
void* memalign(size_t, size_t);
int posix_memalign(void**, size_t, size_t);
void* valloc(size_t);
void* pvalloc(size_t);


// Communication with the environment
//
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 127

_Noreturn void abort( void );
int atexit( void (*func)(void) );
int at_quick_exit( void (*func)(void) );
_Noreturn void exit( int status );
_Noreturn void _Exit( int status );
char *getenv( const char *name );
_Noreturn void quick_exit( int status );
int system( const char *string );

// Searching and sorting utilities
//
typedef int (*__cmp)( const void *, const void *);

void *bsearch( const void *key, const void *base, size_t nmemb, size_t size, __cmp compar );
void qsort( const void *base, size_t nmemb, size_t size, __cmp compar );

// Integer arithmetic functions
//
struct div_t { int quot; int rem; };
struct ldiv_t { long quot; long rem; };
struct lldiv_t { long long quot; long long rem; };

int abs( int j );
long labs( long j );
long long llabs( long long j );

struct div_t div( int numer, int denom );
struct ldiv_t ldiv( long numer, long denom );
struct lldiv_t lldiv( long long numer, long long denom );

// Multibyte / wide character conversion functions
//
#define MB_CUR_MAX SIZE_C(4)

int mblen( const char *s, size_t n );
int mbtowc( wchar_t * restrict pwc, const char * restrict s, size_t n );
int wctomb( char *s, wchar_t wc );
int mbstowcs( wchar_t * restrict pwcs, const char * restrict s, size_t n );
int wcstombs( char * restrict s, const wchar_t * restrict pwcs, size_t n );

#ifdef __cplusplus
} // extern "C"
#endif
