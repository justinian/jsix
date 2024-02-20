#pragma once
/** \file wchar.h
  * Extended mutlibyte and wide character utilities
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#include <__j6libc/file.h>
#include <__j6libc/null.h>
#include <__j6libc/restrict.h>
#include <__j6libc/size_t.h>
#include <__j6libc/wchar_t.h>
#include <stdarg.h>

struct tm;

typedef struct {
} mbstate_t;

#ifdef __cplusplus
extern "C" {
#endif

// Formatted wide character input/output functions
//
int wprintf(const wchar_t * restrict format, ...);
int vwprintf(const wchar_t * restrict format, va_list arg);

int fwprintf(FILE * restrict stream, const wchar_t * restrict format, ...);
int vfwprintf(FILE * restrict stream, const wchar_t * restrict format, va_list arg);

int swprintf(wchar_t * restrict s, size_t n, const wchar_t * restrict format, ...);
int vswprintf(wchar_t * restrict s, size_t n, const wchar_t * restrict format, va_list arg);

int wscanf(const wchar_t * restrict format, ...);
int vwscanf(const wchar_t * restrict format, va_list arg);

int fwscanf(FILE * restrict stream, const wchar_t * restrict format, ...);
int vfwscanf(FILE * restrict stream, const wchar_t * restrict format, va_list arg);

int swscanf(const wchar_t * restrict s, const wchar_t * restrict format, ...);
int vswscanf(const wchar_t * restrict s, const wchar_t * restrict format, va_list arg);

// Wide character input/output functions
//
wint_t fgetwc(FILE * stream);
wchar_t * fgetws(wchar_t * restrict s, size_t n, FILE * stream);

wint_t fputwc(wchar_t c, FILE * stream);
int fputws(const wchar_t * restrict s, FILE * stream);

int fwide(FILE * stream, int mode);

wint_t getwc(FILE * stream);
wint_t getwchar(void);

wint_t putwc(wchar_t c, FILE * stream);
wint_t putwchar(wchar_t c);

wint_t ungetwc(wchar_t c, FILE * stream);

// General wide string utilities: numeric conversion functions
//
double wcstod(const wchar_t * restrict nptr, wchar_t ** restrict endptr);
float wcstof(const wchar_t * restrict nptr, wchar_t ** restrict endptr);
long double wcstold(const wchar_t * restrict nptr, wchar_t ** restrict endptr);

long wcstol(const wchar_t * restrict nptr, wchar_t ** restrict endptr, int base);
long long wcstoll(const wchar_t * restrict nptr, wchar_t ** restrict endptr, int base);
unsigned long wcstoul(const wchar_t * restrict nptr, wchar_t ** restrict endptr, int base);
unsigned long long wcstoull(const wchar_t * restrict nptr, wchar_t ** restrict endptr, int base);

// General wide string utilities: numeric conversion functions
//
wchar_t * wcscpy(wchar_t * restrict s1, const wchar_t * restrict s2);
wchar_t * wcsncpy(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
wchar_t * wmemcpy(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
wchar_t * wmemmove(wchar_t * s1, const wchar_t * s2, size_t n);

// General wide string utilities: concatenation functions
//
wchar_t * wcscat(wchar_t * restrict s1, const wchar_t * restrict s2);
wchar_t * wcsncat(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);

// General wide string utilities: comparison functions
//
int wcscmp(const wchar_t * s1, const wchar_t * s2);
int wcscoll(const wchar_t * s1, const wchar_t * s2);
int wcsncmp(const wchar_t * s1, const wchar_t * s2, size_t n);
size_t wcsxfrm(wchar_t * restrict s1, const wchar_t * restrict s2, size_t n);
int wmemcmp(const wchar_t * s1, const wchar_t * s2, size_t n);

// General wide string utilities: search functions
//
wchar_t * wcschr(const wchar_t * s, wchar_t c);
size_t wcscspn(const wchar_t * s1, const wchar_t * s2);
wchar_t * wcspbrk(const wchar_t * s1, const wchar_t * s2);
wchar_t * wcsrchr(const wchar_t * s, wchar_t c);
size_t wcsspn(const wchar_t * s1, const wchar_t * s2);
wchar_t * wcsstr(const wchar_t * s1, const wchar_t * s2);
wchar_t * wcstok(wchar_t * restrict s1, const wchar_t * restrict s2, wchar_t ** restrict ptr);
wchar_t * wmemchr(const wchar_t * s, wchar_t c, size_t n);

// Miscellaneous functions
//
size_t wcslen(const wchar_t * s);
wchar_t wmemset(wchar_t * s, wchar_t c, size_t n);

// Wide character time conversion functions
//
size_t wcsftime(wchar_t * restrict s, size_t maxsize, const wchar_t * restrict format,
        const struct tm * restrict timeptr);

// Extended multi-byte/wide character conversion utilities
//
wint_t btowc(int c);
int wctob(wint_t c);

int mbsinit(const mbstate_t *ps);

size_t mbrlen(const char * restrict s, size_t n, mbstate_t * restrict ps);
size_t mbrtowc(wchar_t * restrict pwc, const char * restrict s, size_t n, mbstate_t * restrict ps);
size_t wcrtomb(char * restrict s, wchar_t wc, mbstate_t * restrict ps);

size_t mbsrtowcs(wchar_t * restrict dst, const char ** restrict src, size_t len, mbstate_t * restrict ps);
size_t wcsrtombs(char * restrict dst, const wchar_t ** restrict src, size_t len, mbstate_t * restrict ps);

#ifdef __cplusplus
} // extern "C"
#endif
