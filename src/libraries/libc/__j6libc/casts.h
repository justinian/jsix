#pragma once
/** \file j6libc/casts.h
  * Internal implementations of casting helpers for C's bad lib types
  *
  * This file is part of the C standard library for the jsix operating
  * system.
  *
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at https://mozilla.org/MPL/2.0/.
  */

#ifndef __cplusplus
#error "__j6libc/casts.h included by non-C++ code"
#endif

namespace __j6libc {

template <typename T> struct non_const           { using type = T; };
template <typename T> struct non_const<T*>       { using type = T*; };
template <typename T> struct non_const<const T>  { using type = T; };
template <typename T> struct non_const<const T*> { using type = T*; };

// cast_to: like reinterpret_cast, with an optional const_cast. Useful
// when C's stdlib wants to _take_ a const pointer, but return that
// pointer as non-const.

/*
template <typename T, typename S>
inline T cast_to(S p) { return reinterpret_cast<T>(p); }
*/

template <typename T, typename S>
inline T cast_to(S p) {
    return reinterpret_cast<T>(const_cast<typename non_const<S>::type>(p));
}

} // namespace __j6libc
