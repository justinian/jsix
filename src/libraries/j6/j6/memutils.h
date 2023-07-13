#pragma once
/// \file memutils.h
/// Standard mem*() library functions

#include <__j6libc/restrict.h>
#include <__j6libc/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void * restrict s1, const void * restrict s2, size_t n);
void *memmove(void * restrict s1, const void * restrict s2, size_t n);
void *memset(void *s, int c, size_t n);

#ifdef __cplusplus
} // extern "C"
#endif