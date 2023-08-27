#pragma once
/// \file memutils.h
/// Standard mem*() library functions

#include <util/api.h>
#include <__j6libc/restrict.h>
#include <__j6libc/size_t.h>

#ifdef __cplusplus
extern "C" {
#endif

void * API memcpy(void * restrict s1, const void * restrict s2, size_t n);
void * API memmove(void * restrict s1, const void * restrict s2, size_t n);
void * API memset(void *s, int c, size_t n);

#ifdef __cplusplus
} // extern "C"
#endif
