#include <chrono>
#include <random>
#include <vector>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include "kutil/memory.h"
#include "kutil/heap_manager.h"
#include "catch.hpp"

using namespace kutil;


void *memory = nullptr;
size_t total_alloc_size = 0;
size_t total_alloc_calls = 0;

const size_t hs = 0x10; // header size
const size_t max_block = 1 << 16;


void grow_callback(void *start, size_t length)
{
	total_alloc_size += length;
	total_alloc_calls += 1;
}


TEST_CASE( "Buddy blocks tests", "[memory buddy]" )
{
	using clock = std::chrono::system_clock;
	unsigned seed = clock::now().time_since_epoch().count();
	std::default_random_engine rng(seed);

	memory = aligned_alloc(max_block, 4 * max_block);

	heap_manager mm(memory, grow_callback);

	// The ctor should have allocated an initial block
	CHECK( total_alloc_size == max_block );
	CHECK( total_alloc_calls == 1 );

	// Blocks should be:
	// 16: 0-64K

	std::vector<void *> allocs(6);
	for (int i = 0; i < 6; ++i)
		allocs[i] = mm.allocate(150); // size 8

	// Should not have grown
	CHECK( total_alloc_size == max_block );
	CHECK( total_alloc_calls == 1 );

	// Blocks should be:
	// 16: [0-64K]
	// 15: [0-32K], 32K-64K
	// 14: [0-16K], 16K-32K
	// 13: [0-8K], 8K-16K
	// 12: [0-4K], 4K-8K
	// 11: [0-2K], 2K-4K
	// 10: [0-1K, 1-2K]
	//  9: [0, 512, 1024], 1536
	//  8: [0, 256, 512, 768, 1024, 1280]

	// We have free memory at 1526 and 2K, but we should get 4K
	void *big = mm.allocate(4000); // size 12
	REQUIRE( big == offset_pointer(memory, 4096 + hs) );
	mm.free(big);

	// free up 512
	mm.free(allocs[3]);
	mm.free(allocs[4]);

	// Blocks should be:
	// ...
	//  9: [0, 512, 1024], 1536
	//  8: [0, 256, 512], 768, 1024, [1280]

	// A request for a 512-block should not cross the buddy divide
	big = mm.allocate(500); // size 9
	REQUIRE( big >= offset_pointer(memory, 1536 + hs) );
	mm.free(big);

	mm.free(allocs[0]);
	mm.free(allocs[1]);
	mm.free(allocs[2]);
	mm.free(allocs[5]);
	allocs.clear();

	std::vector<size_t> sizes = {
		16000, 8000, 4000, 4000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 150,
		150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 48, 48, 48, 13 };

	std::shuffle(sizes.begin(), sizes.end(), rng);

	allocs.reserve(sizes.size());
	for (size_t size : sizes)
		allocs.push_back(mm.allocate(size));

	std::shuffle(allocs.begin(), allocs.end(), rng);
	for (void *p: allocs)
		mm.free(p);
	allocs.clear();

	big = mm.allocate(64000);

	// If everything was freed / joined correctly, that should not have allocated
	CHECK( total_alloc_size == max_block );
	CHECK( total_alloc_calls == 1 );

	// And we should have gotten back the start of memory
	CHECK( big == offset_pointer(memory, hs) );
}
