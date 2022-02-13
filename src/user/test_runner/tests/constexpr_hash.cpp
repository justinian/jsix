#include <util/constexpr_hash.h>
#include "test_case.h"

class hash_tests :
    public test::fixture
{
};

TEST_CASE( hash_tests, equality_test )
{
    const unsigned hash1 = static_cast<unsigned>("hash1!"_h);
    CHECK( hash1 == 210, "hash gave unexpected value");

    const unsigned hash2 = static_cast<unsigned>("hash1!"_h);
    CHECK(hash1 == hash2, "hashes of equal strings should be equal");

    const unsigned hash3 = static_cast<unsigned>("not hash1!"_h);
    CHECK(hash1 != hash3, "hashes of different strings should not be equal");
    CHECK(hash3 == 37, "hash gave unexpected value");

    const unsigned hash4 = static_cast<unsigned>("another thing that's longer"_h);
    CHECK(hash1 != hash4, "hashes of different strings should not be equal");
    CHECK(hash4 == 212, "hash gave unexpected value");
}
