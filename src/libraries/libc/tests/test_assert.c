#include "_PDCLIB_test.h"

#define NDEBUG
#include <assert.h>

#define enabled_assert( expression ) ( ( expression ) ? (void) 0 \
        : _PDCLIB_assert( "Assertion failed: " #expression \
                          ", function ", __func__, \
                          ", file " __FILE__ \
                          ", line " _PDCLIB_symbol2string( __LINE__ ) \
                          "." _PDCLIB_endl ) )

static int EXPECTED = 0;
static void aborthandler( int sig )
{
	_exit(!EXPECTED);
}

static int assert_disabled_test( void )
{
    int i = 0;
    assert( i == 0 ); /* NDEBUG set, condition met */
    assert( i == 1 ); /* NDEBUG set, condition fails */
    return i;
}

int assert_enabled_test(int val)
{
	enabled_assert(val);
	return val;
}

START_TEST( assert )
{
    sighandler_t sh = signal(SIGABRT, &aborthandler);
    TESTCASE( sh != SIG_ERR );

    TESTCASE( assert_disabled_test() == 0 );
    TESTCASE( assert_enabled_test(1) ); /* NDEBUG not set, condition met */

	EXPECTED = 1;
    TESTCASE( assert_enabled_test(0) ); /* NDEBUG not set, condition fails - should abort */
}
END_TEST

START_SUITE( assert )
{
	RUN_TEST( assert );
}
END_SUITE
