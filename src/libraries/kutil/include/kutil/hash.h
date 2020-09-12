#pragma once
/// \file hash.h
/// Simple templated hashing functions

#include <stddef.h>
#include <stdint.h>

namespace kutil {

constexpr uint64_t fnv_64_prime = 0x100000001b3ull;
constexpr uint64_t fnv1a_64_init = 0xcbf29ce484222325ull;

/// Return the FNV-1a hash of the given 0-terminated string.
inline uint64_t hash_string(char const *s, uint64_t init = 0) {
	if (!init) init = fnv1a_64_init;
	while(s && *s) {
		init ^= static_cast<uint64_t>(*s++);
		init *= fnv_64_prime;
	}
	return init;
}

/// Return the FNV-1a hash of the given buffer.
inline uint64_t hash_buffer(const void *v, size_t len, uint64_t init = 0) {
	uint8_t const *p = reinterpret_cast<uint8_t const*>(v);
	uint8_t const *end = p + len;
	if (!init) init = fnv1a_64_init;
	while(p < end) {
		init ^= static_cast<uint64_t>(*p++);
		init *= fnv_64_prime;
	}
	return init;
}

template <typename T>
uint64_t hash(const T &v) {
	return hash_buffer(reinterpret_cast<const void*>(&v), sizeof(T));
} 

template <>
uint64_t hash<const char *>(const char * const &s) {
	return hash_string(s);
}

} // namespace kutil
