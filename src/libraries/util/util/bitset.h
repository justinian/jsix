#pragma once
/// \file bitset.h
/// Definition of the `bitset` template class

#include <stdint.h>

namespace util {

/// A statically-sized templated bitset
template <unsigned N>
class bitset
{
    static constexpr unsigned num_elems = (N + 63) / 64;

public:
    template <typename T>
    __attribute__ ((force_inline))
    inline bool get(T i) const {
        return bits(i) & bit(i);
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bitset & set(T i) {
        bits(i) |= bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bitset & clear(T i) {
        bits(i) &= ~bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bool operator[](T i) const { return get(i); }

    inline bool empty() const {
        for (uint64_t i = 0; i < num_elems; ++i)
            if (m_bits[i]) return false;
        return true;
    }

private:
    template <typename T>
    __attribute__ ((force_inline))
    inline uint64_t bit(T i) const { return (1ull << (static_cast<uint64_t>(i) & 63)); }

    template <typename T>
    __attribute__ ((force_inline))
    inline uint64_t &bits(T i) { return m_bits[static_cast<uint64_t>(i) >> 6]; }

    template <typename T>
    __attribute__ ((force_inline))
    inline uint64_t bits(T i) const { return m_bits[static_cast<uint64_t>(i) >> 6]; }

    uint64_t m_bits[num_elems] = {0};
};

namespace {
}

/// A statically-sized templated bitset
template <>
class bitset<64>
{
    template <typename T>
    static constexpr uint64_t bit_or(T b) { return 1ull << uint64_t(b); }

    template <typename T, typename ...Args>
    static constexpr uint64_t bit_or(T b, Args... bs) { return (1ull << uint64_t(b)) | bit_or(bs...); }

public:
    bitset(uint64_t v = 0) : m_bits {v} {}

    bitset(const bitset<64> &o) : m_bits {o.m_bits} {}

    template <typename ...Args>
    constexpr explicit bitset(Args... args) : m_bits(bit_or(args...)) {}

    template <typename T>
    __attribute__ ((force_inline))
    inline bitset & operator=(T v) { m_bits = static_cast<uint64_t>(v); return *this; }

    inline constexpr operator const uint64_t () const { return m_bits; }

    template <typename T>
    __attribute__ ((force_inline))
    inline constexpr bool get(T i) const {
        return m_bits & bit(i);
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bitset & set(T i) {
        m_bits |= bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bitset & clear(T i) {
        m_bits &= ~bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline constexpr bool operator[](T i) const { return get(i); }

    inline constexpr bool empty() const { return m_bits == 0; }

    inline constexpr uint64_t value() const { return m_bits; }

private:
    template <typename T>
    inline constexpr uint64_t bit(T i) const { return (1ull << static_cast<uint64_t>(i)); }

    uint64_t m_bits;
};

/// A statically-sized templated bitset
template <>
class bitset<32>
{
    template <typename T>
    static constexpr uint32_t bit_or(T b) { return 1u << uint32_t(b); }

    template <typename T, typename ...Args>
    static constexpr uint32_t bit_or(T b, Args... bs) { return (1u << uint32_t(b)) | bit_or(bs...); }

public:
    bitset(uint32_t v = 0) : m_bits {v} {}

    bitset(const bitset<32> &o) : m_bits {o.m_bits} {}

    template <typename ...Args>
    constexpr bitset(Args... args) : m_bits(bit_or(args...)) {}

    template <typename T>
    inline bitset & operator=(T v) { m_bits = static_cast<uint32_t>(v); return *this; }

    inline constexpr operator uint32_t () const { return m_bits; }

    template <typename T>
    __attribute__ ((force_inline))
    inline constexpr bool get(T i) const {
        return m_bits & bit(i);
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bitset & set(T i) {
        m_bits |= bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bitset & clear(T i) {
        m_bits &= ~bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((force_inline))
    inline bool operator[](T i) const { return get(i); }

    inline bool empty() const { return m_bits == 0; }

    inline constexpr uint32_t value() const { return m_bits; }

private:
    template <typename T>
    inline uint32_t bit(T i) const { return (1u << static_cast<uint32_t>(i)); }

    uint32_t m_bits;
};

using bitset64 = bitset<64>;
using bitset32 = bitset<32>;

} // namespace util
