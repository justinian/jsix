#pragma once
/// \file xoroshiro.h
/// xoroshiro256++ random number implementation

#include <stdint.h>

namespace util
{

class xoroshiro256pp
{
public:
    static constexpr size_t state_words = 4;

    /// Constructor.
    /// \arg seed  A non-zero seed for the generator
    xoroshiro256pp(uint64_t seed);

    /// Constructor with explicit state.
    xoroshiro256pp(const uint64_t state[state_words]);

    /// Copy constructor
    xoroshiro256pp(const xoroshiro256pp &other);

    /// Assignment operator
    xoroshiro256pp & operator=(const xoroshiro256pp &other);

    /// Get the next random integer from the generator
    /// \returns  A random integer between 0-2^64, inclusive
    uint64_t next();

    /// This is the jump function for the generator. It is equivalent
    /// to 2^128 calls to next(); it can be used to generate 2^128
    /// non-overlapping subsequences for parallel computations.
    /// \returns  A new generator 2^128 calls ahead of this one
    inline xoroshiro256pp jump() const { return do_jump(jump_poly); }

    /// This is the long-jump function for the generator. It is
    /// equivalent to 2^192 calls to next(); it can be used to
    /// generate 2^64 starting points, from each of which jump()
    /// will generate 2^64 non-overlapping subsequences for
    /// parallel distributed computations.
    /// \returns  A new generator 2^192 calls ahead of this one
    xoroshiro256pp long_jump() const { return do_jump(longjump_poly); }

private:
    static const uint64_t jump_poly[state_words];
    static const uint64_t longjump_poly[state_words];

    xoroshiro256pp do_jump(const uint64_t poly[state_words]) const;

    uint64_t m_state[state_words];
};

} // namespace util