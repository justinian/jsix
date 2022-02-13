#include <vector>
#include <util/map.h>

#include "test_case.h"
#include "test_rng.h"

struct map_tests :
    public test::fixture
{
};

TEST_CASE( map_tests, insert )
{
    test::rng rng {12345};

    std::vector<int> ints;
    for (int i = 0; i < 1000; ++i)
        ints.push_back(i);

    size_t sizes[] = {1, 2, 10, 50, 100, 100};
    for (size_t s : sizes) {
        util::map<int, int> v;
        std::shuffle(ints.begin(), ints.end(), rng);

        for (int i = 0; i < s; ++i) {
            v.insert(ints[i], ints[i]);
        }

        for (int i = 0; i < s; ++i) {
            int *p = v.find(ints[i]);
            CHECK( p, "Map did not have expected key" );
            CHECK( *p == ints[i], "Map did not have expected value" );
        }
    }
}

TEST_CASE( map_tests, deletion )
{
    test::rng rng {12345};

    std::vector<int> ints;
    for (int i = 0; i < 1000; ++i)
        ints.push_back(i);

    size_t sizes[] = {1, 2, 3, 5, 100};
    for (size_t s : sizes) {
        util::map<int, int> v;
        std::shuffle(ints.begin(), ints.end(), rng);

        for (int i = 0; i < s; ++i) {
            v.insert(ints[i], ints[i]);
        }

        for (int i = 0; i < s; i += 2) {
            v.erase(ints[i]);
        }

        for (int i = 0; i < s; ++i) {
            int *p = v.find(ints[i]);
            if ( i%2 )
                CHECK( p, "Expected map item did not exist" );
            else
                CHECK( !p, "Deleted item should not exist" );
        }
    }
}

TEST_CASE( map_tests, pointer_vals )
{
    util::map<int, int*> v;
    int is[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; ++i)
        v.insert(i*7, &is[i]);

    for (int i = 0; i < 4; ++i) {
        int *p = v.find(i*7);
        CHECK( p, "Expected pointer did not exist" );
        CHECK( p == &is[i], "Expected pointer was not correct" );
    }

    CHECK( v.find(3) == nullptr, "Expected empty slot exists" );
}

TEST_CASE( map_tests, uint64_keys )
{
    util::map<uint64_t, int> v;
    int is[4] = { 2, 3, 5, 7 };
    for (uint64_t i = 0; i < 4; ++i)
        v.insert(i+1, is[i]);

    for (uint64_t i = 0; i < 4; ++i) {
        int *p = v.find(i+1);
        CHECK( p, "Expected integer did not exist" );
        CHECK( *p == is[i], "Expected integer was not correct" );
    }

    CHECK( v.find(30) == nullptr, "Expected missing intger was found" );
}
