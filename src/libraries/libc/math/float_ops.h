#pragma once
#include <stddef.h>
#include <stdint.h>

struct single_precision
{
    using type = float;
    static constexpr size_t sig_bits = 23;
    static constexpr size_t exp_bits =  8;
};

struct double_precision
{
    using type = double;
    static constexpr size_t sig_bits = 52;
    static constexpr size_t exp_bits = 11;
};

template<typename Traits>
inline typename Traits::type __ceil(typename Traits::type f) {
    static constexpr int64_t exp_mask = (1ll<<Traits::exp_bits) - 1;
    static constexpr int64_t exp_mid = (1ll<<(Traits::exp_bits-1)) - 1;

    uint64_t bits = *reinterpret_cast<uint64_t*>(&f);
    int64_t exponent = static_cast<int64_t>((bits >> Traits::sig_bits) & exp_mask) - exp_mid;
    if (exponent < 0)
        return f > 0 ? 1 : 0;

    int64_t fraction_bits = Traits::sig_bits - exponent;
    if (fraction_bits <= 0)
        return f;

    uint64_t nonfraction_mask = -1ull << fraction_bits;
    uint64_t nonfraction = bits & nonfraction_mask;
    typename Traits::type rounded = *reinterpret_cast<typename Traits::type*>(&nonfraction);

    if (rounded > 0 && bits != nonfraction)
        rounded += 1;

    return rounded;
}

template<typename Traits>
inline typename Traits::type __frexp(typename Traits::type f, int *exp) {
    static constexpr int64_t exp_mask = ((1ll<<Traits::exp_bits) - 1) << Traits::sig_bits;
    static constexpr int64_t exp_mid = (1ll<<(Traits::exp_bits-1)) - 1;

    uint64_t bits = *reinterpret_cast<uint64_t*>(&f);

    if (bits == 0) {
        *exp = 0;
        return 0;
    }

    *exp = ((bits & exp_mask) >> Traits::sig_bits) - exp_mid;

    uint64_t result = (bits & ~exp_mask) | (exp_mid << Traits::sig_bits);
    return *reinterpret_cast<typename Traits::type*>(&result);
}
