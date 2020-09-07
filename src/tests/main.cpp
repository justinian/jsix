#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "kutil/assert.h"

bool ASSERT_EXPECTED = false;
bool ASSERT_HAPPENED = false;
void test_assert(const char *file, unsigned line, const char *message)
{
	CAPTURE( file );
	CAPTURE( line );
	INFO( message );
	REQUIRE( ASSERT_EXPECTED );
	ASSERT_EXPECTED = false;
	ASSERT_HAPPENED = true;
}


int main( int argc, char* argv[] ) {
	kutil::assert_set_callback(test_assert);

	int result = Catch::Session().run( argc, argv );
	return result;
}

namespace kutil {
void * kalloc(size_t size) { return malloc(size); }
void kfree(void *p) { return free(p); }
}
