#pragma once
/// \file bitset.h
/// Definition of the `bitset` template class

#include <stdint.h>

namespace util {

/// A statically-sized templated bitset
template <typename I>
class bitset
{
public:
    using storage_type = I;

private:
    template <typename T>
    static constexpr storage_type bit_or(T b) { return 1ull << uint64_t(b); }

    template <typename T, typename ...Args>
    static constexpr storage_type bit_or(T b, Args... bs) { return (1ull << storage_type(b)) | bit_or(bs...); }

public:
    constexpr bitset(storage_type v = 0) : m_bits {v} {}

    constexpr bitset(const bitset<I> &o) : m_bits {o.m_bits} {}

    template <typename ...Args>
    __attribute__ ((always_inline))
    static constexpr bitset of(Args... args) { return {bit_or(args...)}; }

    template <typename T>
    __attribute__ ((always_inline))
    static constexpr bitset from(T i) { return {storage_type(i)}; }

    __attribute__ ((always_inline))
    inline constexpr bitset & operator=(bitset b) { m_bits = b.m_bits; return *this; }

    inline constexpr operator storage_type () const { return m_bits; }

    template <typename T>
    __attribute__ ((always_inline))
    inline constexpr bool get(T i) const {
        return m_bits & bit(i);
    }

    template <typename T>
    __attribute__ ((always_inline))
    inline bitset & set(T i) {
        m_bits |= bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((always_inline))
    inline bitset & clear(T i) {
        m_bits &= ~bit(i);
        return *this;
    }

    __attribute__ ((always_inline))
    inline storage_type range(unsigned start, unsigned count) {
        return (m_bits >> start) & ~((1 << count) - 1);
    }

    __attribute__ ((always_inline))
    inline bitset & set_range(unsigned start, unsigned count, storage_type val) {
        const storage_type mask =  ~((1 << count) - 1) << start;
        m_bits = (m_bits & ~mask) | ((val << start) & mask);
        return *this;
    }

    template <typename T>
    __attribute__ ((always_inline))
    inline constexpr bool operator[](T i) const { return get(i); }

    __attribute__ ((always_inline))
    inline constexpr bitset operator|(bitset b) const { return {storage_type(m_bits | b.m_bits)}; }

    template <typename T>
    __attribute__ ((always_inline))
    inline constexpr bitset operator+(T i) const { return {storage_type(m_bits | bit(i))}; }

    __attribute__ ((always_inline))
    inline constexpr bitset operator|=(bitset b) { *this = *this|b; return *this; }

    template <typename T>
    __attribute__ ((always_inline))
    inline constexpr bitset operator+=(T i) { set(i); return *this; }

    __attribute__ ((always_inline))
    inline constexpr bitset operator&(const bitset &b) const { return {storage_type(m_bits & b.m_bits)}; }

    template <typename T>
    __attribute__ ((always_inline))
    inline constexpr bitset operator&(T i) const { return {m_bits & bit(i)}; }

    __attribute__ ((always_inline))
    inline constexpr bitset & operator&=(const bitset &b) { m_bits &= b.m_bits; return *this; }

    __attribute__ ((always_inline))
    inline constexpr bool operator==(const bitset &b) const { return m_bits == b.m_bits; }

    template <typename T>
    __attribute__ ((always_inline))
    inline constexpr bool operator==(T i) const { return m_bits == storage_type(i); }

    inline constexpr bool empty() const { return m_bits == 0; }

    inline constexpr uint64_t value() const { return m_bits; }

private:
    template <typename T>
    inline constexpr storage_type bit(T i) const { return (storage_type(1) << int(i)); }

    storage_type m_bits;
};

using bitset64 = bitset<uint64_t>;
using bitset32 = bitset<uint32_t>;
using bitset16 = bitset<uint16_t>;
using bitset8 = bitset<uint8_t>;

template <unsigned N>
class sized_bitset
{
    static constexpr unsigned num_elems = (N + 63) / 64;

public:
    template <typename T>
    __attribute__ ((always_inline))
    inline bool get(T i) const {
        return bits(i) & bit(i);
    }

    template <typename T>
    __attribute__ ((always_inline))
    inline sized_bitset & set(T i) {
        bits(i) |= bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((always_inline))
    inline sized_bitset & clear(T i) {
        bits(i) &= ~bit(i);
        return *this;
    }

    template <typename T>
    __attribute__ ((always_inline))
    inline bool operator[](T i) const { return get(i); }

    inline bool empty() const {
        for (uint64_t i = 0; i < num_elems; ++i)
            if (m_bits[i]) return false;
        return true;
    }

private:
    template <typename T>
    __attribute__ ((always_inline))
    inline uint64_t bit(T i) const { return (1ull << (static_cast<uint64_t>(i) & 63)); }

    template <typename T>
    __attribute__ ((always_inline))
    inline uint64_t &bits(T i) { return m_bits[static_cast<uint64_t>(i) >> 6]; }

    template <typename T>
    __attribute__ ((always_inline))
    inline uint64_t bits(T i) const { return m_bits[static_cast<uint64_t>(i) >> 6]; }

    uint64_t m_bits[num_elems] = {0};
};

} // namespace util
