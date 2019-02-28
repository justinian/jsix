#include <chrono>
#include <random>
#include <vector>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include "kutil/address_manager.h"
#include "kutil/heap_manager.h"
#include "kutil/memory.h"
#include "catch.hpp"

using namespace kutil;

extern void * grow_callback(size_t);
extern void free_memory();

const size_t max_block = 1ull << 36;
const size_t start = max_block;
const size_t GB = 1ull << 30;

TEST_CASE( "Buddy addresses tests", "[address buddy]" )
{
	heap_manager mm(grow_callback);
	kutil::setup::set_heap(&mm);

	using clock = std::chrono::system_clock;
	unsigned seed = clock::now().time_since_epoch().count();
	std::default_random_engine rng(seed);

	address_manager am;
	am.add_regions(start, max_block * 2);

	// Blocks should be:
	// 36: 0-64G, 64-128G

	uintptr_t a = am.allocate(0x4000); // under 64K min
	uintptr_t b = am.allocate(0x4000);
	CHECK( b == a + (1<<16));

	am.free(a);
	am.free(b);

	// Should be back to
	// 36: 0-64G, 64-128G

	a = am.allocate(max_block);
	CHECK(a == start);
	am.free(a);

	// Should be back to
	// 36: 0-64G, 64-128G

	// This should allocate the 66-68G block, not the
	// 66-67G block.
	a = am.mark( start + 66 * GB + 0x1000, GB );
	CHECK( a == start + 66 * GB );

	// Free blocks should be:
	// 36: 0-64G
	// 35: 96-128G
	// 34: 80-96G
	// 33: 72-80G
	// 32: 68-72G
	// 31: 64-66G

	// This should cause a split of the one 31 block, NOT
	// be a leftover of the above mark.
	b = am.allocate(GB);
	CHECK( b == start + 64 * GB );
	am.free(b);
	am.free(a);

	// Should be back to
	// 36: 0-64G, 64-128G
	a = am.allocate(max_block);
	b = am.allocate(max_block);
	CHECK( b == start + max_block );
	CHECK( a == start );

	kutil::setup::set_heap(nullptr);
	free_memory();
}
