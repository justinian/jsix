#pragma once
/// \file util.h
/// Utility functions used in other kutil code

#include <stdint.h>

namespace kutil {

// Get the base-2 logarithm of i
inline unsigned log2(uint64_t i) {
    if (i < 2) return 0;
    const unsigned clz = __builtin_clzll(i - 1);
    return 64 - clz;
}

}
