// PRNG based on xoshiro256++ 1.0 by David Blackman and Sebastiano Vigna.
// https://prng.di.unimi.it/xoshiro256plusplus.c

/* This is xoshiro256++ 1.0, an all-purpose, rock-solid generator by
   David Blackman and Sebastiano Vigna.

   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill the state. */


#include <util/hash.h>
#include <util/xoroshiro.h>

namespace util {

const uint64_t xoroshiro256pp::jump_poly[] = { 0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c };
const uint64_t xoroshiro256pp::longjump_poly[] = { 0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635 };

static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

xoroshiro256pp::xoroshiro256pp(uint64_t seed)
{
    m_state[0] = seed;
    m_state[1] = splitmix64(m_state[0]);
    m_state[2] = splitmix64(m_state[1]);
    m_state[3] = splitmix64(m_state[2]);
}

xoroshiro256pp::xoroshiro256pp(const uint64_t state[state_words]) :
    m_state {state[0], state[1], state[2], state[3]}
{
}

xoroshiro256pp::xoroshiro256pp(const xoroshiro256pp &other) :
    m_state {other.m_state[0], other.m_state[1], other.m_state[2], other.m_state[3]}
{
}

xoroshiro256pp &
xoroshiro256pp::operator=(const xoroshiro256pp &other)
{
    m_state[0] = other.m_state[0];
    m_state[1] = other.m_state[1];
    m_state[2] = other.m_state[2];
    m_state[3] = other.m_state[3];
    return *this;
}

uint64_t
xoroshiro256pp::next()
{
    const uint64_t result = rotl(m_state[0] + m_state[3], 23) + m_state[0];

    const uint64_t t = m_state[1] << 17;

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= t;

    m_state[3] = rotl(m_state[3], 45);

    return result;
}

xoroshiro256pp
xoroshiro256pp::do_jump(const uint64_t poly[state_words]) const
{
	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
    xoroshiro256pp x = *this;

	for(int i = 0; i < state_words; ++i) {
		for(int b = 0; b < 64; ++b) {
			if (poly[i] & (1ull << b)) {
				s0 ^= x.m_state[0];
				s1 ^= x.m_state[1];
				s2 ^= x.m_state[2];
				s3 ^= x.m_state[3];
			}
			x.next();	
		}
	}	

	x.m_state[0] = s0;
	x.m_state[1] = s1;
	x.m_state[2] = s2;
	x.m_state[3] = s3;
    return x;
}

} // namespace util