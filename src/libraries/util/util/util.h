#pragma once
/// \file util.h
/// Utility functions used in other util code

#include <stdint.h>

namespace util {

constexpr size_t const_log2(size_t n) {
    return n < 2 ? 1 : 1 + const_log2(n/2);
}

constexpr bool is_pow2(size_t n) {
    return (n & (n-1)) == 0;
}

// Get the base-2 logarithm of i
inline unsigned log2(uint64_t i) {
    if (i < 2) return 0;
    const unsigned clz = __builtin_clzll(i - 1);
    return 64 - clz;
}

}
