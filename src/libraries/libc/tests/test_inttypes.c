#include "_PDCLIB_test.h"

START_TEST( imaxabs )
{
    TESTCASE( imaxabs( (intmax_t)0 ) == 0 );
    TESTCASE( imaxabs( INTMAX_MAX ) == INTMAX_MAX );
    TESTCASE( imaxabs( INTMAX_MIN + 1 ) == -( INTMAX_MIN + 1 ) );
}
END_TEST

START_TEST( imaxdiv )
{
    imaxdiv_t result;
    result = imaxdiv( (intmax_t)5, (intmax_t)2 );
    TESTCASE( result.quot == 2 && result.rem == 1 );
    result = imaxdiv( (intmax_t)-5, (intmax_t)2 );
    TESTCASE( result.quot == -2 && result.rem == -1 );
    result = imaxdiv( (intmax_t)5, (intmax_t)-2 );
    TESTCASE( result.quot == -2 && result.rem == 1 );
    TESTCASE( sizeof( result.quot ) == sizeof( intmax_t ) );
    TESTCASE( sizeof( result.rem )  == sizeof( intmax_t ) );
}
END_TEST

START_TEST( strtoimax )
{
    char * endptr;
    /* this, to base 36, overflows even a 256 bit integer */
    char overflow[] = "-ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ_";
    /* tricky border case */
    char tricky[] = "+0xz";
    errno = 0;

    /* basic functionality */
    TESTCASE( strtoimax( "123", NULL, 10 ) == 123 );
    /* proper detecting of default base 10 */
    TESTCASE( strtoimax( "456", NULL, 0 ) == 456 );
    /* proper functioning to smaller base */
    TESTCASE( strtoimax( "14", NULL, 8 ) == 12 );
    /* proper autodetecting of octal */
    TESTCASE( strtoimax( "016", NULL, 0 ) == 14 );
    /* proper autodetecting of hexadecimal, lowercase 'x' */
    TESTCASE( strtoimax( "0xFF", NULL, 0 ) == 255 );
    /* proper autodetecting of hexadecimal, uppercase 'X' */
    TESTCASE( strtoimax( "0Xa1", NULL, 0 ) == 161 );

    /* proper handling of border case: 0x followed by non-hexdigit */
    TESTCASE(
        (strtoimax(tricky, &endptr, 0) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* proper handling of border case: 0 followed by non-octdigit */
    TESTCASE(
        (strtoimax(tricky, &endptr, 8) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* overflowing subject sequence must still return proper endptr */
    TESTCASE(
        (strtoimax(overflow, &endptr, 36) == INTMAX_MIN) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* same for positive */
    TESTCASE(
        (strtoimax(overflow + 1, &endptr, 36) == INTMAX_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* testing skipping of leading whitespace */
    TESTCASE( strtoimax( " \n\v\t\f789", NULL, 0 ) == 789 );

    /* testing conversion failure */
    TESTCASE( (strtoimax(overflow, &endptr, 10) == 0) && (endptr == overflow) );
    TESTCASE( (strtoimax(overflow, &endptr, 0) == 0) && (endptr == overflow) );
    /* These tests assume two-complement, but conversion should work for   */
    /* one-complement and signed magnitude just as well. Anyone having a   */
    /* platform to test this on?                                           */
#if INTMAX_MAX >> 62 == 1
    /* testing "odd" overflow, i.e. base is not a power of two */
    TESTCASE( (strtoimax("9223372036854775807", NULL, 0) == INTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoimax("9223372036854775808", NULL, 0) == INTMAX_MAX) && (errno == ERANGE) );
    TESTCASE( (strtoimax("-9223372036854775807", NULL, 0) == (INTMAX_MIN + 1)) && (errno == 0) );
    TESTCASE( (strtoimax("-9223372036854775808", NULL, 0) == INTMAX_MIN) && (errno == 0) );
    TESTCASE( (strtoimax("-9223372036854775809", NULL, 0) == INTMAX_MIN) && (errno == ERANGE) );
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoimax("0x7fffffffffffffff", NULL, 0) == INTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoimax("0x8000000000000000", NULL, 0 ) == INTMAX_MAX) && (errno == ERANGE) );
    TESTCASE( (strtoimax("-0x7fffffffffffffff", NULL, 0 ) == (INTMAX_MIN + 1)) && (errno == 0) );
    TESTCASE( (strtoimax("-0x8000000000000000", NULL, 0 ) == INTMAX_MIN) && (errno == 0) );
    TESTCASE( (strtoimax("-0x8000000000000001", NULL, 0 ) == INTMAX_MIN) && (errno == ERANGE) );
#elif LLONG_MAX >> 126 == 1
    /* testing "odd" overflow, i.e. base is not a power of two */
    TESTCASE( (strtoimax("170141183460469231731687303715884105728", NULL, 0 ) == INTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoimax("170141183460469231731687303715884105729", NULL, 0 ) == INTMAX_MAX) && (errno == ERANGE) );
    TESTCASE( (strtoimax("-170141183460469231731687303715884105728", NULL, 0 ) == (INTMAX_MIN + 1)) && (errno == 0) );
    TESTCASE( (strtoimax("-170141183460469231731687303715884105729", NULL, 0 ) == INTMAX_MIN) && (errno == 0) );
    TESTCASE( (strtoimax("-170141183460469231731687303715884105730", NULL, 0 ) == INTMAX_MIN) && (errno == ERANGE) );
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoimax("0x7fffffffffffffffffffffffffffffff", NULL, 0 ) == INTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoimax("0x80000000000000000000000000000000", NULL, 0 ) == INTMAX_MAX) && (errno == ERANGE) );
    TESTCASE( (strtoimax("-0x7fffffffffffffffffffffffffffffff", NULL, 0 ) == (INTMAX_MIN + 1)) && (errno == 0) );
    TESTCASE( (strtoimax("-0x80000000000000000000000000000000", NULL, 0 ) == INTMAX_MIN) && (errno == 0) );
    TESTCASE( (strtoimax("-0x80000000000000000000000000000001", NULL, 0 ) == INTMAX_MIN) && (errno == ERANGE) );
#else
#error Unsupported width of 'intmax_t' (neither 64 nor 128 bit).
#endif
}
END_TEST

START_TEST( strtoumax )
{
    char * endptr;
    /* this, to base 36, overflows even a 256 bit integer */
    char overflow[] = "-ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ_";
    /* tricky border case */
    char tricky[] = "+0xz";
    errno = 0;

    /* basic functionality */
    TESTCASE( strtoumax( "123", NULL, 10 ) == 123 );
    /* proper detecting of default base 10 */
    TESTCASE( strtoumax( "456", NULL, 0 ) == 456 );
    /* proper functioning to smaller base */
    TESTCASE( strtoumax( "14", NULL, 8 ) == 12 );
    /* proper autodetecting of octal */
    TESTCASE( strtoumax( "016", NULL, 0 ) == 14 );
    /* proper autodetecting of hexadecimal, lowercase 'x' */
    TESTCASE( strtoumax( "0xFF", NULL, 0 ) == 255 );
    /* proper autodetecting of hexadecimal, uppercase 'X' */
    TESTCASE( strtoumax( "0Xa1", NULL, 0 ) == 161 );

    /* proper handling of border case: 0x followed by non-hexdigit */
    TESTCASE(
        (strtoumax(tricky, &endptr, 0) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* proper handling of border case: 0 followed by non-octdigit */
    TESTCASE(
        (strtoumax(tricky, &endptr, 8) == 0) &&
        (endptr == tricky + 2) &&
        (errno == 0) );

    /* overflowing subject sequence must still return proper endptr */
    TESTCASE(
        (strtoumax(overflow, &endptr, 36) == UINTMAX_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* same for positive */
    TESTCASE(
        (strtoumax(overflow + 1, &endptr, 36) == UINTMAX_MAX) &&
        (errno == ERANGE) &&
        ((endptr - overflow) == 53) );

    /* testing skipping of leading whitespace */
    TESTCASE( strtoumax( " \n\v\t\f789", NULL, 0 ) == 789 );

    /* testing conversion failure */
    TESTCASE( (strtoumax(overflow, &endptr, 10) == 0) && (endptr == overflow) );
    TESTCASE( (strtoumax(overflow, &endptr, 0) == 0) && (endptr == overflow) );
/* uintmax_t -> long long -> 64 bit */
#if UINTMAX_MAX >> 63 == 1
    /* testing "odd" overflow, i.e. base is not power of two */
    TESTCASE( (strtoumax("18446744073709551615", NULL, 0) == UINTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoumax("18446744073709551616", NULL, 0) == UINTMAX_MAX) && (errno == ERANGE) );
    /* testing "even" overflow, i.e. base is power of two */
    TESTCASE( (strtoumax("0xFFFFFFFFFFFFFFFF", NULL, 0) == UINTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoumax("0x10000000000000000", NULL, 0) == UINTMAX_MAX) && (errno == ERANGE) );
/* uintmax_t -> long long -> 128 bit */
#elif UINTMAX_MAX >> 127 == 1
    /* testing "odd" overflow, i.e. base is not power of two */
    TESTCASE( (strtoumax("340282366920938463463374607431768211455", NULL, 0) == UINTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoumax("340282366920938463463374607431768211456", NULL, 0) == UINTMAX_MAX) && (errno == ERANGE) );
    /* testing "even" everflow, i.e. base is power of two */
    TESTCASE( (strtoumax("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", NULL, 0) == UINTMAX_MAX) && (errno == 0) );
    TESTCASE( (strtoumax("0x100000000000000000000000000000000", NULL, 0) == UINTMAX_MAX) && (errno == ERANGE) );
#else
#error Unsupported width of 'uintmax_t' (neither 64 nor 128 bit).
#endif
}
END_TEST


START_SUITE( inttypes )
{
	RUN_TEST( imaxabs );
	RUN_TEST( imaxdiv );
	RUN_TEST( strtoimax );
	RUN_TEST( strtoumax );
}
END_SUITE
