#pragma once
/** \file j6libc/bits.h
  * Internal helpers for bitwise math.
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#ifndef __cplusplus
#error "__j6libc/bits.h included by non-C++ code"
#endif

#include <stdint.h>
#include <__j6libc/size_t.h>

namespace __j6libc {

constexpr size_t log2(size_t n) {
    return n < 2 ? 1 : 1 + log2(n/2);
}

constexpr size_t word_bytes = sizeof(void*);
constexpr size_t word_bits  = word_bytes << 3;
constexpr uintptr_t word_align_mask  = word_bytes - 1;
constexpr unsigned word_shift = log2(word_bytes);

template <typename T>
inline bool is_aligned(uintptr_t addr) {
    return (addr & (sizeof(T)-1)) == 0;
}

template <typename T, typename S>
inline bool is_aligned(S *p) {
    return is_aligned<T>(reinterpret_cast<uintptr_t>(p));
}

} // namespace __j6libc
