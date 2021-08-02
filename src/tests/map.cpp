#include "kutil/map.h"
#include "catch.hpp"

using Catch::rng;
std::uniform_int_distribution<int> distrib {0, 10000};

TEST_CASE( "map insertion", "[containers] [map]" )
{
    std::vector<int> ints;
    for (int i = 0; i < 1000; ++i)
        ints.push_back(i);

    size_t sizes[] = {1, 2, 3, 5, 100};
    for (size_t s : sizes) {
        kutil::map<int, int> v;
        std::shuffle(ints.begin(), ints.end(), rng());

        for (int i = 0; i < s; ++i) {
            v.insert(ints[i], ints[i]);
        }

        for (int i = 0; i < s; ++i) {
            int *p = v.find(ints[i]);
            CAPTURE( s );
            CAPTURE( i );
            CAPTURE( ints[i] );
            CAPTURE( kutil::hash(ints[i]) );
            CHECK( p );
            CHECK( *p == ints[i] );
        }
    }
}

TEST_CASE( "map deletion", "[containers] [map]" )
{
    std::vector<int> ints;
    for (int i = 0; i < 1000; ++i)
        ints.push_back(i);

    size_t sizes[] = {1, 2, 3, 5, 100};
    for (size_t s : sizes) {
        kutil::map<int, int> v;
        std::shuffle(ints.begin(), ints.end(), rng());

        for (int i = 0; i < s; ++i) {
            v.insert(ints[i], ints[i]);
        }

        for (int i = 0; i < s; i += 2) {
            v.erase(ints[i]);
        }

        for (int i = 0; i < s; ++i) {
            int *p = v.find(ints[i]);
            CAPTURE( s );
            CAPTURE( i );
            CAPTURE( ints[i] );
            CAPTURE( kutil::hash(ints[i]) );
            if ( i%2 )
                CHECK( p );
            else
                CHECK( !p );
        }
    }
}

TEST_CASE( "map with pointer vals", "[containers] [map]" )
{
    kutil::map<int, int*> v;
    int is[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; ++i)
        v.insert(i*7, &is[i]);

    for (int i = 0; i < 4; ++i) {
        int *p = v.find(i*7);
        CHECK( p == &is[i] );
    }

    CHECK( v.find(3) == nullptr );
}

TEST_CASE( "map with uint64_t keys", "[containers] [map]" )
{
    kutil::map<uint64_t, int> v;
    int is[4] = { 2, 3, 5, 7 };
    for (uint64_t i = 0; i < 4; ++i)
        v.insert(i+1, is[i]);

    for (uint64_t i = 0; i < 4; ++i) {
        int *p = v.find(i+1);
        CHECK( *p == is[i] );
    }

    CHECK( v.find(30) == nullptr );
}
