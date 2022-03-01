#pragma once
/// \file hash.h
/// Simple templated hashing functions

#include <stddef.h>
#include <stdint.h>
#include <util/basic_types.h>

namespace util {
namespace fnv1a {

constexpr uint32_t val_32 = 0x811c9dc5;
constexpr uint32_t prime_32 = 0x1000193;
constexpr uint64_t val_64 = 0xcbf29ce484222325;
constexpr uint64_t prime_64 = 0x100000001b3;

/// Evaluate the 32-bit constexpr FNV-1a hash of the given 0-terminated string.
inline constexpr uint32_t hash32_const(const char* const str, uint32_t value = val_32) {
    return (str[0] == '\0') ? value :
        hash32_const(&str[1], (value ^ uint32_t(str[0])) * prime_32);
}

/// Evaluate the 64-bit constexpr FNV-1a hash of the given 0-terminated string.
inline constexpr uint64_t hash64_const(const char* const str, uint64_t value = val_64) {
    return (str[0] == '\0') ? value :
        hash64_const(&str[1], (value ^ uint64_t(str[0])) * prime_64);
}

/// Return the 64-bit FNV-1a hash of the given 0-terminated string.
inline uint64_t hash64_string(char const *s, uint64_t value = val_64) {
    while(s && *s) {
        value ^= static_cast<uint64_t>(*s++);
        value *= prime_64;
    }
    return value;
}

/// Return the 64-bit FNV-1a hash of the given buffer.
inline uint64_t hash64(void const * const v, size_t len, uint64_t value = val_64) {
    uint8_t const *p = reinterpret_cast<uint8_t const*>(v);
    uint8_t const *end = p + len;
    while(p < end) {
        value ^= static_cast<uint64_t>(*p++);
        value *= prime_64;
    }
    return value;
}

} // namespace fnv1a

template <unsigned N> 
constexpr inline typename types::sized_uint<N>::type hash_fold(typename types::sized_uint<N*2>::type value) {
    return (value >> types::sized_uint<N>::bits) ^ (value & types::sized_uint<N>::mask);
}

template <unsigned N> 
constexpr inline typename types::sized_uint<N>::type hash_fold(typename types::sized_uint<N*4>::type value) {
    typename types::sized_uint<N*2>::type value2 = hash_fold<N*2>(value);
    return (value2 >> types::sized_uint<N>::bits) ^ (value2 & types::sized_uint<N>::mask);
}

template <typename T>
inline uint64_t hash(const T &v) { return fnv1a::hash64(reinterpret_cast<const void*>(&v), sizeof(T)); } 

template <> inline uint64_t hash<uint8_t> (const uint8_t &i)  { return i; }
template <> inline uint64_t hash<uint16_t>(const uint16_t &i) { return i; }
template <> inline uint64_t hash<uint32_t>(const uint32_t &i) { return i; }
template <> inline uint64_t hash<uint64_t>(const uint64_t &i) { return i; }
template <> inline uint64_t hash<const char *>(const char * const &s) { return fnv1a::hash64_string(s); }

} // namespace util

constexpr inline uint64_t operator "" _id (const char *s, size_t len) { return util::fnv1a::hash64_const(s); }
constexpr inline uint8_t operator "" _id8 (const char *s, size_t len) { return util::hash_fold<8>(util::fnv1a::hash32_const(s)); }
