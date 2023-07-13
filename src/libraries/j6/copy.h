#pragma once
/// \file copy.h
/// Internal implementations to aid in implementing mem* functions

#include <stddef.h>

namespace j6 {

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

} // namespace j6
