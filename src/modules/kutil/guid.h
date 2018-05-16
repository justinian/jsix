#pragma once
/// \file guid.h
/// Definition of the guid type and related functions
#include <stdint.h>
#include "kutil/misc.h"

namespace kutil {


/// A GUID
struct guid
{
	uint64_t a, b;
};

/// Make a GUID by writing it naturally-ordered in code:
///  AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE becomes:
///  make_guid(0xAAAAAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEEEEEEEEEE);
/// \returns  The guid object
inline constexpr guid make_guid(uint32_t a, uint16_t b, uint16_t c, uint16_t d, uint64_t e)
{
	const uint64_t h =
		static_cast<uint64_t>(c) << 48 |
		static_cast<uint64_t>(b) << 32 |
		a;

	const uint64_t l =
		static_cast<uint64_t>(byteswap(e & 0xffffffff)) << 32 |
		(byteswap(e >> 32) & 0xffff0000) |
		((d << 8) & 0xff00) | ((d >> 8) & 0xff);

	return {h, l};
}

} // namespace kutil


inline bool operator==(const kutil::guid &a, const kutil::guid &b) { return a.a == b.a && a.b == b.b; }
