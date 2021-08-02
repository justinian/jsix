#include <vector>
#include "kutil/vector.h"
#include "catch.hpp"
#include "container_helpers.h"

TEST_CASE( "sorted vector tests", "[containers] [vector]" )
{
    using clock = std::chrono::system_clock;
    unsigned seed = clock::now().time_since_epoch().count();
    std::default_random_engine rng(seed);
    std::uniform_int_distribution<int> distrib(0,10000);

    kutil::vector<sortableT> v;

    int sizes[] = {1, 2, 3, 5, 100};
    for (int s : sizes) {
        for (int i = 0; i < s; ++i) {
            sortableT t { distrib(rng) };
            v.sorted_insert(t);
        }

        for (int i = 1; i < s; ++i)
            CHECK( v[i].value >= v[i-1].value );
    }
}
