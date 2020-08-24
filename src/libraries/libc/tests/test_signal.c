#include "_PDCLIB_test.h"
#include <stdlib.h>

static volatile sig_atomic_t flag = 0;

static int expected_signal = 0;
static int received_signal = 0;

static void test_handler( int sig )
{
    flag = 1;
}

START_TEST( raise )
{
    /* Could be other than SIG_DFL if you changed the implementation. */
    TESTCASE( signal( SIGABRT, SIG_IGN ) == SIG_DFL );
    /* Should be ignored. */
    TESTCASE( raise( SIGABRT ) == 0 );
    /* Installing test handler, old handler should be returned */
    TESTCASE( signal( SIGABRT, test_handler ) == SIG_IGN );
    /* Raising and checking SIGABRT */
    expected_signal = SIGABRT;
    TESTCASE( raise( SIGABRT ) == 0 );
    TESTCASE( flag == 1 );
    TESTCASE( received_signal == SIGABRT );
    /* Re-installing test handler, should have been reset to default */
    /* Could be other than SIG_DFL if you changed the implementation. */
    TESTCASE( signal( SIGABRT, test_handler ) == SIG_DFL );
    /* Raising and checking SIGABRT */
    flag = 0;
    TESTCASE( raise( SIGABRT ) == 0 );
    TESTCASE( flag == 1 );
    TESTCASE( received_signal == SIGABRT );
    /* Installing test handler for different signal... */
    TESTCASE( signal( SIGTERM, test_handler ) == SIG_DFL );
    /* Raising and checking SIGTERM */
    expected_signal = SIGTERM;
    TESTCASE( raise( SIGTERM ) == 0 );
    TESTCASE( flag == 1 );
    TESTCASE( received_signal == SIGTERM );
}
END_TEST

START_SUITE( signal )
{
	RUN_TEST( raise );
}
END_SUITE
