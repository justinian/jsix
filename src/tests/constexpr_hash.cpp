#include "kutil/constexpr_hash.h"
#include "catch.hpp"

using namespace kutil;

TEST_CASE( "constexpr hash", "[hash]" )
{
	const unsigned hash1 = static_cast<unsigned>("hash1!"_h);
	CHECK(hash1 == 210);

	const unsigned hash2 = static_cast<unsigned>("hash1!"_h);
	CHECK(hash1 == hash2);

	const unsigned hash3 = static_cast<unsigned>("not hash1!"_h);
	CHECK(hash1 != hash3);
	CHECK(hash3 == 37);

	const unsigned hash4 = static_cast<unsigned>("another thing that's longer"_h);
	CHECK(hash1 != hash4);
	CHECK(hash4 == 212);
}
