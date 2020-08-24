#include "_PDCLIB_test.h"
#include <assert.h>


static int test_exit() { _Exit(0); return 0; }

START_TEST( _Exit )
{
    TESTCASE( test_exit() );
}
END_TEST

static void aborthandler(int s) { _exit(0); }
static int test_abort() { abort(); return 0; }

START_TEST( abort )
{
    TESTCASE( signal( SIGABRT, &aborthandler ) != SIG_ERR );
    TESTCASE( test_abort() );
}
END_TEST

START_TEST( abs )
{
    TESTCASE( abs( 0 ) == 0 );
    TESTCASE( abs( INT_MAX ) == INT_MAX );
    TESTCASE( abs( INT_MIN + 1 ) == -( INT_MIN + 1 ) );
}
END_TEST

static int flags[ 32 ];

static void counthandler( void )
{
    static int count = 0;
    flags[ count ] = count;
    ++count;
}

static void checkhandler( void )
{
    for ( int i = 0; i < 31; ++i )
        assert( flags[ i ] == i );
}

int test_atexit()
{
    if( atexit( &checkhandler ) != 0 )
        return 0;

    for ( int i = 0; i < 31; ++i )
       if( atexit( &counthandler ) != 0 )
           return 0;

    return 1;
}

START_TEST( atexit )
{
    TESTCASE( test_atexit() );
}
END_TEST

static int compare( const void * left, const void * right )
{
    return *( (unsigned char *)left ) - *( (unsigned char *)right );
}

START_TEST( bsearch )
{
    TESTCASE( bsearch( "e", abcde, 4, 1, compare ) == NULL );
    TESTCASE( bsearch( "e", abcde, 5, 1, compare ) == &abcde[4] );
    TESTCASE( bsearch( "a", abcde + 1, 4, 1, compare ) == NULL );
    TESTCASE( bsearch( "0", abcde, 1, 1, compare ) == NULL );
    TESTCASE( bsearch( "a", abcde, 1, 1, compare ) == &abcde[0] );
    TESTCASE( bsearch( "a", abcde, 0, 1, compare ) == NULL );
    TESTCASE( bsearch( "e", abcde, 3, 2, compare ) == &abcde[4] );
    TESTCASE( bsearch( "b", abcde, 3, 2, compare ) == NULL );
}
END_TEST

START_TEST( div )
{
    div_t result = div( 5, 2 );
    TESTCASE( result.quot == 2 && result.rem == 1 );
    result = div( -5, 2 );
    TESTCASE( result.quot == -2 && result.rem == -1 );
    result = div( 5, -2 );
    TESTCASE( result.quot == -2 && result.rem == 1 );
    TESTCASE( sizeof( result.quot ) == sizeof( int ) );
    TESTCASE( sizeof( result.rem )  == sizeof( int ) );
}
END_TEST

START_TEST( getenv )
{
    TESTCASE( strcmp( getenv( "SHELL" ), "/bin/bash" ) == 0 );
}
END_TEST

START_TEST( labs )
{
    TESTCASE( labs( 0 ) == 0 );
    TESTCASE( labs( LONG_MAX ) == LONG_MAX );
    TESTCASE( labs( LONG_MIN + 1 ) == -( LONG_MIN + 1 ) );
}
END_TEST

START_TEST( ldiv )
{
    ldiv_t result = ldiv( 5, 2 );
    TESTCASE( result.quot == 2 && result.rem == 1 );
    result = ldiv( -5, 2 );
    TESTCASE( result.quot == -2 && result.rem == -1 );
    result = ldiv( 5, -2 );
    TESTCASE( result.quot == -2 && result.rem == 1 );
    TESTCASE( sizeof( result.quot ) == sizeof( long ) );
    TESTCASE( sizeof( result.rem )  == sizeof( long ) );
}
END_TEST

START_TEST( llabs )
{
    TESTCASE( llabs( 0ll ) == 0 );
    TESTCASE( llabs( LLONG_MAX ) == LLONG_MAX );
    TESTCASE( llabs( LLONG_MIN + 1 ) == -( LLONG_MIN + 1 ) );
}
END_TEST

START_TEST( lldiv )
{
    lldiv_t result = lldiv( 5ll, 2ll );
    TESTCASE( result.quot == 2 && result.rem == 1 );
    result = lldiv( -5ll, 2ll );
    TESTCASE( result.quot == -2 && result.rem == -1 );
    result = lldiv( 5ll, -2ll );
    TESTCASE( result.quot == -2 && result.rem == 1 );
    TESTCASE( sizeof( result.quot ) == sizeof( long long ) );
    TESTCASE( sizeof( result.rem )  == sizeof( long long ) );
}
END_TEST

START_TEST( qsort )
{
    char presort[] = { "shreicnyjqpvozxmbt" };
    char sorted1[] = { "bcehijmnopqrstvxyz" };
    char sorted2[] = { "bticjqnyozpvreshxm" };
    char s[19];
    strcpy( s, presort );
    qsort( s, 18, 1, compare );
    TESTCASE( strcmp( s, sorted1 ) == 0 );
    strcpy( s, presort );
    qsort( s, 9, 2, compare );
    TESTCASE( strcmp( s, sorted2 ) == 0 );
    strcpy( s, presort );
    qsort( s, 1, 1, compare );
    TESTCASE( strcmp( s, presort ) == 0 );
    qsort( s, 100, 0, compare );
    TESTCASE( strcmp( s, presort ) == 0 );
}
END_TEST

START_TEST( rand )
{
    int rnd1 = RAND_MAX, rnd2 = RAND_MAX;
    TESTCASE( ( rnd1 = rand() ) < RAND_MAX );
    TESTCASE( ( rnd2 = rand() ) < RAND_MAX );
    srand( 1 );
    TESTCASE( rand() == rnd1 );
    TESTCASE( rand() == rnd2 );
}
END_TEST

START_TEST( strtol )
{
    char * endptr = NULL;
    /* this, to base 36, overflows even a 256 bit integer */
    char overflow[] = "-ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ_";
    /* tricky border case */
    char tricky[] = "+0xz";
    errno = 0;

    /* basic functionality */
    TESTCASE( strtol( "123", NULL, 10 ) == 123 );
    /* proper detecting of default base 10 */
    TESTCASE( strtol( "456", NULL, 0 ) == 456 );
    /* proper functioning to smaller base */
    TESTCASE( strtol( "14", NULL, 8 ) == 12 );
    /* proper autodetecting of octal */
    TESTCASE( strtol( "016", NULL, 0 ) == 14 );
    /* proper autodetecting of hexadecimal, lowercase 'x' */
    TESTCASE( strtol( "0xFF", NULL, 0 ) == 255 );
    /* proper autodetecting of hexadecimal, uppercase 'X' */
    TESTCASE( strtol( "0Xa1", NULL, 0 ) == 161 );

    /* proper handling of border case: 0x followed by non-hexdigit */
    TESTCASE(
        (strtol(tricky, &endptr, 0) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* proper handling of border case: 0 followed by non-octdigit */
    TESTCASE(
        (strtol(tricky, &endptr, 8) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* overflowing subject sequence must still return proper endptr */
    TESTCASE(
        (strtol(overflow, &endptr, 36) == LONG_MIN) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* same for positive */
    TESTCASE( 
        (strtol(overflow + 1, &endptr, 36) == LONG_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* testing skipping of leading whitespace */
    TESTCASE( strtol( " \n\v\t\f789", NULL, 0 ) == 789 );

    /* testing conversion failure */
    TESTCASE(
        (strtol(overflow, &endptr, 10) == 0) &&
        (endptr == overflow) );

    TESTCASE(
        (strtol(overflow, &endptr, 0) == 0) &&
        (endptr == overflow) );

#if __SIZEOF_LONG__ == 4
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtol("2147483647", NULL, 0) == 0x7fffffff) && (errno == 0) );
    TESTCASE( (strtol("2147483648", NULL, 0) == LONG_MAX) && (errno == ERANGE) );
    TESTCASE( (strtol("-2147483647", NULL, 0) == (long)0x80000001) && (errno == 0) );
    TESTCASE( (strtol("-2147483648", NULL, 0) == LONG_MIN) && (errno == 0) );
    TESTCASE( (strtol("-2147483649", NULL, 0) == LONG_MIN) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
#elif __SIZEOF_LONG__ == 8
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtol("9223372036854775807", NULL, 0) == 0x7fffffffffffffff) && (errno == 0) );
    TESTCASE( (strtol("9223372036854775808", NULL, 0) == LONG_MAX) && (errno == ERANGE) );
    TESTCASE( (strtol("-9223372036854775807", NULL, 0) == (long)0x8000000000000001) && (errno == 0) );
    TESTCASE( (strtol("-9223372036854775808", NULL, 0) == LONG_MIN) && (errno == 0) );
    TESTCASE( (strtol("-9223372036854775809", NULL, 0) == LONG_MIN) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
#else
#error Unsupported width of 'long' (neither 32 nor 64 bit).
#endif
}
END_TEST

START_TEST( strtoll )
{
    char * endptr = NULL;
    /* this, to base 36, overflows even a 256 bit integer */
    char overflow[] = "-ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ_";
    /* tricky border case */
    char tricky[] = "+0xz";
    errno = 0;

    /* basic functionality */
    TESTCASE( strtoll( "123", NULL, 10 ) == 123 );
    /* proper detecting of default base 10 */
    TESTCASE( strtoll( "456", NULL, 0 ) == 456 );
    /* proper functioning to smaller base */
    TESTCASE( strtoll( "14", NULL, 8 ) == 12 );
    /* proper autodetecting of octal */
    TESTCASE( strtoll( "016", NULL, 0 ) == 14 );
    /* proper autodetecting of hexadecimal, lowercase 'x' */
    TESTCASE( strtoll( "0xFF", NULL, 0 ) == 255 );
    /* proper autodetecting of hexadecimal, uppercase 'X' */
    TESTCASE( strtoll( "0Xa1", NULL, 0 ) == 161 );

    /* proper handling of border case: 0x followed by non-hexdigit */
    TESTCASE(
        (strtoll(tricky, &endptr, 0) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* proper handling of border case: 0 followed by non-octdigit */
    TESTCASE(
        (strtoll(tricky, &endptr, 8) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* overflowing subject sequence must still return proper endptr */
    TESTCASE(
        (strtoll(overflow, &endptr, 36) == LLONG_MIN) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* same for positive */
    TESTCASE(
        (strtoll(overflow + 1, &endptr, 36) == LLONG_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* testing skipping of leading whitespace */
    TESTCASE( strtoll( " \n\v\t\f789", NULL, 0 ) == 789 );

    /* testing conversion failure */
    TESTCASE( (strtoll(overflow, &endptr, 10) == 0) && (endptr == overflow) );
    TESTCASE( (strtoll(overflow, &endptr, 0) == 0) && (endptr == overflow) );

    /* TODO: These tests assume two-complement, but conversion should work */
    /* for one-complement and signed magnitude just as well. Anyone having */
    /* a platform to test this on?                                         */
#if __SIZEOF_LONG_LONG__ == 8
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoll("9223372036854775807", NULL, 0) == 0x7fffffffffffffff) && (errno == 0) );
    TESTCASE( (strtoll("9223372036854775808", NULL, 0) == LLONG_MAX) && (errno == ERANGE) );
    TESTCASE( (strtoll("-9223372036854775807", NULL, 0) == (long long)0x8000000000000001) && (errno == 0) );
    TESTCASE( (strtoll("-9223372036854775808", NULL, 0) == LLONG_MIN) && (errno == 0) );
    TESTCASE( (strtoll("-9223372036854775809", NULL, 0) == LLONG_MIN) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
#elif __SIZEOF_LONG_LONG__ == 16
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoll("170141183460469231731687303715884105728", NULL, 0) == 0x7fffffffffffffffffffffffffffffff) && (errno == 0) );
    TESTCASE( (strtoll("170141183460469231731687303715884105729", NULL, 0) == LLONG_MAX) && (errno == ERANGE) );
    TESTCASE( (strtoll("-170141183460469231731687303715884105728", NULL, 0) == -0x80000000000000000000000000000001) && (errno == 0) );
    TESTCASE( (strtoll("-170141183460469231731687303715884105729", NULL, 0) == LLONG_MIN) && (errno == 0) );
    TESTCASE( (strtoll("-170141183460469231731687303715884105730", NULL, 0) == LLONG_MIN) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
#else
#error Unsupported width of 'long long' (neither 64 nor 128 bit).
#endif
}
END_TEST

START_TEST( strtoul )
{
    char * endptr = NULL;
    /* this, to base 36, overflows even a 256 bit integer */
    char overflow[] = "-ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ_";
    /* tricky border case */
    char tricky[] = "+0xz";
    errno = 0;
    /* basic functionality */
    TESTCASE( strtoul( "123", NULL, 10 ) == 123 );
    /* proper detecting of default base 10 */
    TESTCASE( strtoul( "456", NULL, 0 ) == 456 );
    /* proper functioning to smaller base */
    TESTCASE( strtoul( "14", NULL, 8 ) == 12 );
    /* proper autodetecting of octal */
    TESTCASE( strtoul( "016", NULL, 0 ) == 14 );
    /* proper autodetecting of hexadecimal, lowercase 'x' */
    TESTCASE( strtoul( "0xFF", NULL, 0 ) == 255 );
    /* proper autodetecting of hexadecimal, uppercase 'X' */
    TESTCASE( strtoul( "0Xa1", NULL, 0 ) == 161 );

    /* proper handling of border case: 0x followed by non-hexdigit */
    TESTCASE(
        (strtoul(tricky, &endptr, 0) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

   /* proper handling of border case: 0 followed by non-octdigit */
     TESTCASE(
        (strtoul(tricky, &endptr, 8) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* overflowing subject sequence must still return proper endptr */
    TESTCASE(
        (strtoul(overflow, &endptr, 36) == ULONG_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* same for positive */
    TESTCASE(
        (strtoul(overflow + 1, &endptr, 36) == ULONG_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* testing skipping of leading whitespace */
    TESTCASE( strtoul( " \n\v\t\f789", NULL, 0 ) == 789 );

    /* testing conversion failure */
    TESTCASE( (strtoul(overflow, &endptr, 10) == 0) && (endptr == overflow) );
    TESTCASE( (strtoul(overflow, &endptr, 0) == 0) && (endptr == overflow) );

    /* TODO: These tests assume two-complement, but conversion should work */
    /* for one-complement and signed magnitude just as well. Anyone having */
    /* a platform to test this on?                                         */
/* long -> 32 bit */
#if __SIZEOF_LONG__ == 4
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoul("4294967295", NULL, 0) == ULONG_MAX) && (errno == 0) );
    TESTCASE( (strtoul("4294967296", NULL, 0) == ULONG_MAX) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
/* long -> 64 bit */
#elif __SIZEOF_LONG__ == 8
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoul("18446744073709551615", NULL, 0) == ULONG_MAX) && (errno == 0) );
    TESTCASE( (strtoul("18446744073709551616", NULL, 0) == ULONG_MAX) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
#else
#error Unsupported width of 'long' (neither 32 nor 64 bit).
#endif
}
END_TEST

START_TEST( strtoull )
{
    char * endptr = NULL;
    /* this, to base 36, overflows even a 256 bit integer */
    char overflow[] = "-ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ_";
    /* tricky border case */
    char tricky[] = "+0xz";
    errno = 0;

    /* basic functionality */
    TESTCASE( strtoull( "123", NULL, 10 ) == 123 );
    /* proper detecting of default base 10 */
    TESTCASE( strtoull( "456", NULL, 0 ) == 456 );
    /* proper functioning to smaller base */
    TESTCASE( strtoull( "14", NULL, 8 ) == 12 );
    /* proper autodetecting of octal */
    TESTCASE( strtoull( "016", NULL, 0 ) == 14 );
    /* proper autodetecting of hexadecimal, lowercase 'x' */
    TESTCASE( strtoull( "0xFF", NULL, 0 ) == 255 );
    /* proper autodetecting of hexadecimal, uppercase 'X' */
    TESTCASE( strtoull( "0Xa1", NULL, 0 ) == 161 );

    /* proper handling of border case: 0x followed by non-hexdigit */
    TESTCASE(
        (strtoull(tricky, &endptr, 0) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* proper handling of border case: 0 followed by non-octdigit */
    TESTCASE(
        (strtoull(tricky, &endptr, 8) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* overflowing subject sequence must still return proper endptr */
    TESTCASE(
        (strtoull(overflow, &endptr, 36) == ULLONG_MAX) && 
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* same for positive */
    TESTCASE(
        (strtoull(overflow + 1, &endptr, 36) == ULLONG_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* testing skipping of leading whitespace */
    TESTCASE( strtoull( " \n\v\t\f789", NULL, 0 ) == 789 );

    /* testing conversion failure */
    TESTCASE( (strtoull(overflow, &endptr, 10) == 0) && (endptr == overflow) );
    TESTCASE( (strtoull(overflow, &endptr, 0) == 0) && (endptr == overflow) );
/* long long -> 64 bit */
#if __SIZEOF_LONG_LONG__ == 8
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoull("18446744073709551615", NULL, 0) == ULLONG_MAX) && (errno == 0) );
    TESTCASE( (strtoull("18446744073709551616", NULL, 0) == ULLONG_MAX) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
/* long long -> 128 bit */
#elif __SIZEOF_LONG_LONG__ == 16
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoull("340282366920938463463374607431768211455", NULL, 0) == ULLONG_MAX) && (errno == 0) );
    TESTCASE( (strtoull("340282366920938463463374607431768211456", NULL, 0) == ULLONG_MAX) && (errno == ERANGE) );
    /* TODO: test "odd" overflow, i.e. base is not power of two */
#else
#error Unsupported width of 'long long' (neither 64 nor 128 bit).
#endif
}
END_TEST


#define SYSTEM_MESSAGE "SUCCESS testing system()"
#define SYSTEM_COMMAND "echo '" SYSTEM_MESSAGE "'"

START_TEST( system )
{
    FILE * fh;
    char buffer[25];
    buffer[24] = 'x';
    TESTCASE( ( fh = freopen( testfile, "wb+", stdout ) ) != NULL );
    TESTCASE( system( SYSTEM_COMMAND ) );
    rewind( fh );
    TESTCASE( fread( buffer, 1, 24, fh ) == 24 );
    TESTCASE( memcmp( buffer, SYSTEM_MESSAGE, 24 ) == 0 );
    TESTCASE( buffer[24] == 'x' );
    TESTCASE( fclose( fh ) == 0 );
    TESTCASE( remove( testfile ) == 0 );
}
END_TEST

START_SUITE( stdlib )
{
	RUN_TEST( _Exit );
	RUN_TEST( abort );
	RUN_TEST( abs );
	RUN_TEST( atexit );
	RUN_TEST( bsearch );
	RUN_TEST( div );
	RUN_TEST( getenv );
	RUN_TEST( labs );
	RUN_TEST( ldiv );
	RUN_TEST( llabs );
	RUN_TEST( lldiv );
	RUN_TEST( qsort );
	RUN_TEST( strtol );
	RUN_TEST( strtoll );
	RUN_TEST( strtoul );
	RUN_TEST( strtoull );
	RUN_TEST( system );
}
END_SUITE
