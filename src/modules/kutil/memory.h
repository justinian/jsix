#pragma once

#include <stddef.h>
#include <stdint.h>


inline void * operator new (size_t, void *p) throw() { return p; }


namespace kutil {

void * memset(void *p, uint8_t v, size_t n);

template <typename T>
T read_from(const void *p) { return *reinterpret_cast<const T *>(p); }

} // namespace kutil
