#include <chrono>
#include <random>
#include <vector>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include "kutil/address_manager.h"
#include "kutil/allocator.h"
#include "catch.hpp"

using namespace kutil;

static const size_t max_block = 1ull << 36;
static const size_t start = max_block;
static const size_t GB = 1ull << 30;

class malloc_allocator :
	public kutil::allocator
{
public:
	virtual void * allocate(size_t n) override { return malloc(n); }
	virtual void free(void *p) override { free(p); }
};

TEST_CASE( "Buddy addresses tests", "[address buddy]" )
{
	malloc_allocator alloc;
	address_manager am(alloc);
	am.add_regions(start, max_block * 2);

	// Blocks should be:
	// 36: 0-64G, 64-128G

	uintptr_t a = am.allocate(0x400); // under min
	uintptr_t b = am.allocate(0x400);
	CHECK( b == a + address_manager::min_alloc);

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
}
