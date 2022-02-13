#include <vector>
#include <util/vector.h>

#include "container_helpers.h"
#include "test_case.h"
#include "test_rng.h"

struct vector_tests :
    public test::fixture
{
};

TEST_CASE( vector_tests, sorted_test )
{
    test::rng rng {12345};

    util::vector<sortableT> v;

    int sizes[] = {1, 2, 3, 5, 100};
    for (int s : sizes) {
        for (int i = 0; i < s; ++i) {
            sortableT t { rng() };
            v.sorted_insert(t);
        }

        for (int i = 1; i < s; ++i)
            CHECK( v[i].value >= v[i-1].value, "v is not sorted" );
    }
}
