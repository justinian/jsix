#pragma once
/** \file j6libc/copy.h
  * Internal implementations to aid in implementing mem* functions
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#ifndef __cplusplus
#error "__j6libc/copy.h included by non-C++ code"
#endif

#include <stddef.h>
#include <__j6libc/size_t.h>

namespace __j6libc {

template <size_t N>
inline void do_copy(char *s1, const char *s2) {
#if __has_builtin(__builtin_memcpy_inline)
    __builtin_memcpy_inline(s1, s2, N);
#else
#warn "Not using __builtin_memcpy_inline for memcpy"
    for (size_t i = 0; i < N; ++i)
        s1[i] = s2[i];
#endif
}

template <size_t N>
inline void do_double_copy(char *s1, const char *s2, size_t n) {
    do_copy<N>(s1, s2);

    ptrdiff_t off = n - N;
    if (!off) return;

    do_copy<N>(s1+off, s2+off);
}

__attribute__((always_inline))
inline void do_large_copy(char *s1, const char *s2, size_t n) {
    asm volatile ("rep movsb" : "+D"(s1), "+S"(s2), "+c"(n) :: "memory");
}


__attribute__((always_inline))
inline void do_backward_copy(char *s1, const char *s2, size_t n) {
    for (size_t i = n - 1; i < n; --i)
        s1[i] = s2[i];
}

} // namespace __j6libc
