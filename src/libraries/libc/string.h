#pragma once
/** \file string.h
  * String handling
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

#ifdef __cplusplus
extern "C" {
#endif

// Copying functions
//
void *memcpy(void * restrict s1, const void * restrict s2, size_t n);
void *memmove(void * restrict s1, const void * restrict s2, size_t n);
char *strncpy(char * restrict s1, const char * restrict s2, size_t n);

// Concatenation functions
//
char *strncat(char * restrict s1, const char * restrict s2, size_t n);

// Comparison functions
//
int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int strcoll(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strxfrm(char * restrict s1, const char * restrict s2, size_t n);

// Search functions
//
void *memchr(const void *s, int c, size_t n);
char *strchr(const char *s, int c);
size_t strcspn(const char *s1, const char *s2);
char *strpbrk(const char *s1, const char *s2);
char *strrchr(const char *s, int c);
size_t strspn(const char *s1, const char *s2);
char *strstr(const char *s1, const char *s2);
char *strtok(char * restrict s1, const char * restrict s2);

// Miscellaneous functions
//
void *memset(void *s, int c, size_t n);
char *strerror(int errnum);
size_t strlen(const char *s);

// Deprecated functions
//
char *strcpy(char * restrict, const char * restrict)
    __attribute__((deprecated("strcpy is unsafe, do not use", "strncpy")));
char *strcat(char * restrict, const char * restrict)
    __attribute__((deprecated("strcat is unsafe, do not use", "strncat")));

#ifdef __cplusplus
} // extern "C"
#endif
