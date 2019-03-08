#include "kutil/frame_allocator.h"
#include "catch.hpp"

using namespace kutil;

TEST_CASE( "Frame allocator tests", "[memory frame]" )
{
	frame_block_list free;
	frame_block_list used;
	frame_block_list cache;

	auto *f = new frame_block_list::item_type;
	f->address = 0x1000;
	f->count = 1;
	f->flags = kutil::frame_block_flags::none;
	free.sorted_insert(f);

	auto *g = new frame_block_list::item_type;
	g->address = 0x2000;
	g->count = 1;
	g->flags = kutil::frame_block_flags::none;
	free.sorted_insert(g);

	frame_allocator fa(std::move(cache));
	fa.init(std::move(free), std::move(used));

	fa.consolidate_blocks();

	uintptr_t a = 0;
	size_t c = fa.allocate(2, &a);
	CHECK( a == 0x1000 );
	CHECK( c == 2 );

	fa.free(a, 2);
	a = 0;

	fa.consolidate_blocks();

	c = fa.allocate(2, &a);
	CHECK( a == 0x1000 );
	CHECK( c == 2 );

	delete f;
	delete g;
}
