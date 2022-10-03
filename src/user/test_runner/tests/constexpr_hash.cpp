#include <util/hash.h>
#include "test_case.h"

constexpr static uint64_t hash1_expected = 0x3d8342e701016873;
constexpr static uint64_t hash3_expected = 0xf0ac589d837f11b8;
constexpr static uint64_t hash4_expected = 0x034742bc87c5c1bc;

class hash_tests :
    public test::fixture
{
};

TEST_CASE( hash_tests, equality_test64 )
{
    const uint64_t hash1 = "hash1!"_id;
    const uint64_t hash2 = "hash1!"_id;
    const uint64_t hash3 = "not hash1!"_id;
    const uint64_t hash4 = "another thing that's longer"_id;

    CHECK( hash1 == hash1_expected, "hash gave unexpected value");
    CHECK( hash1 == hash2, "hashes of equal strings should be equal");
    CHECK( hash1 != hash3, "hashes of different strings should not be equal");
    CHECK( hash3 == hash3_expected, "hash gave unexpected value");
    CHECK( hash1 != hash4, "hashes of different strings should not be equal");
    CHECK( hash4 == hash4_expected, "hash gave unexpected value");
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
