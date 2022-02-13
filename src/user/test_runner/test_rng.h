#pragma once
/// \file test_rng.h
/// Simple xorshift-based psuedorandom number generator for tests

#include <stdint.h>

namespace test {

class rng
{
public:
    using result_type = uint64_t;

    rng(uint64_t seed = 1) : a(seed) {}

    uint64_t operator()() {
        a ^= a << 13;
        a ^= a >> 7;
        a ^= a << 17;
        return a;
    }

    constexpr static uint64_t max() { return UINT64_MAX; }
    constexpr static uint64_t min() { return 0; }

private:
    uint64_t a;
};

} // namespace test
