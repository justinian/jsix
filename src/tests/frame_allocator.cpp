#include "kutil/frame_allocator.h"
#include "kutil/heap_manager.h"
#include "kutil/memory.h"
#include "catch.hpp"

using namespace kutil;

extern void * grow_callback(size_t);
extern void free_memory();

const size_t max_block = 1ull << 36;
const size_t start = max_block;
const size_t GB = 1ull << 30;

TEST_CASE( "Frame allocator tests", "[memory frame]" )
{
	heap_manager mm(grow_callback);
	kutil::setup::set_heap(&mm);

	frame_block_list free;
	frame_block_list used;
	frame_block_list cache;

	auto *f = new frame_block_list::item_type;
	f->address = 0x1000;
	f->count = 1;
	free.sorted_insert(f);

	f = new frame_block_list::item_type;
	f->address = 0x2000;
	f->count = 1;
	free.sorted_insert(f);

	frame_allocator fa(std::move(cache));
	fa.init(std::move(free), std::move(used));

	uintptr_t a = 0;
	size_t c = fa.allocate(2, &a);
	CHECK( a == 0x1000 );
	CHECK( c == 2 );

	fa.free(a, 2);
	a = 0;

	c = fa.allocate(2, &a);
	CHECK( a == 0x1000 );
	CHECK( c == 2 );

	kutil::setup::set_heap(nullptr);
	free_memory();
}
