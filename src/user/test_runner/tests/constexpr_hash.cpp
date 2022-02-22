#include <util/hash.h>
#include "test_case.h"

class hash_tests :
    public test::fixture
{
};

TEST_CASE( hash_tests, equality_test64 )
{
    const auto hash1 = static_cast<unsigned>("hash1!"_id);
    CHECK( hash1 == 210, "hash gave unexpected value");

    const auto hash2 = static_cast<unsigned>("hash1!"_id);
    CHECK(hash1 == hash2, "hashes of equal strings should be equal");

    const auto hash3 = static_cast<unsigned>("not hash1!"_id);
    CHECK(hash1 != hash3, "hashes of different strings should not be equal");
    CHECK(hash3 == 37, "hash gave unexpected value");

    const auto hash4 = static_cast<unsigned>("another thing that's longer"_id);
    CHECK(hash1 != hash4, "hashes of different strings should not be equal");
    CHECK(hash4 == 212, "hash gave unexpected value");
}

TEST_CASE( hash_tests, equality_test8 )
{
    const uint8_t type_ids[] = {
    	"system"_id8,

		"event"_id8,
		"channel"_id8,
		"endpoint"_id8,
		"mailbox"_id8,

		"vma"_id8,

		"process"_id8,
		"thread"_id8
    };
    constexpr unsigned id_count = sizeof(type_ids) / sizeof(type_ids[0]);

    const uint8_t system = "system"_id8;

    CHECK_BARE( type_ids[0] == system );
    for (unsigned i = 0; i < id_count; ++i) {
        for (unsigned j = i + 1; j < id_count; ++j) {
            CHECK_BARE( type_ids[i] != type_ids[j] );
        }
    }
}
