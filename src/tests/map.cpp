#include "kutil/map.h"
#include "catch.hpp"

TEST_CASE( "map insertion", "[containers] [vector]" )
{
	using clock = std::chrono::system_clock;
	unsigned seed = clock::now().time_since_epoch().count();
	std::default_random_engine rng {seed};
	std::uniform_int_distribution<int> distrib {0, 10000};


	size_t sizes[] = {1, 2, 3, 5, 100};
	for (size_t s : sizes) {
		kutil::map<int, int> v;
		std::vector<int> r;

		for (int i = 0; i < s; ++i) {
			int j = distrib(rng);
			r.push_back(j);
			v.insert(j, j);
		}

		for (int i : r) {
			int *p = v.find(i);
			CAPTURE( i );
			CHECK( p );
			CHECK( *p == i );
		}
	}
}

TEST_CASE( "map deletion", "[containers] [vector]" )
{
	using clock = std::chrono::system_clock;
	unsigned seed = clock::now().time_since_epoch().count();
	std::default_random_engine rng {seed};
	std::uniform_int_distribution<int> distrib {0, 10000};

	size_t sizes[] = {1, 2, 3, 5, 100};
	for (size_t s : sizes) {
		kutil::map<int, int> v;
		std::vector<int> r;

		for (int i = 0; i < s; ++i) {
			int j = distrib(rng);
			r.push_back(j);
			v.insert(j, j);
		}

		for (int i = 0; i < s; i += 2) {
			v.erase(r[i]);
		}

		for (int i = 0; i < s; ++i) {
			int *p = v.find(r[i]);
			CAPTURE( i );
			if ( i%2 )
				CHECK( p );
			else
				CHECK( !p );
		}
	}
}
