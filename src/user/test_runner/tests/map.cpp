#include <vector>
#include <util/map.h>
#include <util/node_map.h>

#include "test_case.h"
#include "test_rng.h"

struct map_tests :
    public test::fixture
{
};

struct map_item
{
    uint64_t key;
    uint64_t value;
};

uint64_t & get_map_key(map_item &mi) { return mi.key; }

TEST_CASE( map_tests, node_map )
{
    util::node_map<uint64_t, map_item> map;
    map.insert({12, 14});
    map.insert({13, 15});
    map.insert({14, 16});
    map.insert({15, 17});
    map.insert({16, 18});
    map.insert({20, 22});
    map.insert({24, 26});

    CHECK( map.count() == 7, "Map returned incorred count()" );

    auto *item = map.find(12);
    CHECK( item, "Did not find inserted item" );
    CHECK( item && item->key == 12 && item->value == 14,
            "Found incorrect item" );

    item = map.find(40);
    CHECK( !item, "Found non-inserted item" );

    bool found = map.erase(12);
    CHECK( found, "Failed to delete inserted item" );

    item = map.find(12);
    CHECK( !item, "Found item after delete" );

    // Force the map to grow
    map.insert({35, 38});
    map.insert({36, 39});
    map.insert({37, 40});

    CHECK( map.count() == 9, "Map returned incorred count()" );

    item = map.find(13);
    CHECK( item, "Did not find inserted item after grow()" );
    CHECK( item && item->key == 13 && item->value == 15,
            "Found incorrect item after grow()" );
}

TEST_CASE( map_tests, insert )
{
    test::rng rng {12345};
    double foo = 1.02345;
    foo *= 4.6e4;

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
