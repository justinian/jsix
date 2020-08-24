#include "_PDCLIB_test.h"

START_TEST( isalnum )
{
    TESTCASE( isalnum( 'a' ) );
    TESTCASE( isalnum( 'z' ) );
    TESTCASE( isalnum( 'A' ) );
    TESTCASE( isalnum( 'Z' ) );
    TESTCASE( isalnum( '0' ) );
    TESTCASE( isalnum( '9' ) );
    TESTCASE( ! isalnum( ' ' ) );
    TESTCASE( ! isalnum( '\n' ) );
    TESTCASE( ! isalnum( '@' ) );
}
END_TEST

START_TEST( isalpha )
{
    TESTCASE( isalpha( 'a' ) );
    TESTCASE( isalpha( 'z' ) );
    TESTCASE( ! isalpha( ' ' ) );
    TESTCASE( ! isalpha( '1' ) );
    TESTCASE( ! isalpha( '@' ) );
}
END_TEST

START_TEST( isblank )
{
    TESTCASE( isblank( ' ' ) );
    TESTCASE( isblank( '\t' ) );
    TESTCASE( ! isblank( '\v' ) );
    TESTCASE( ! isblank( '\r' ) );
    TESTCASE( ! isblank( 'x' ) );
    TESTCASE( ! isblank( '@' ) );
}
END_TEST

START_TEST( iscntrl )
{
    TESTCASE( iscntrl( '\a' ) );
    TESTCASE( iscntrl( '\b' ) );
    TESTCASE( iscntrl( '\n' ) );
    TESTCASE( ! iscntrl( ' ' ) );
}
END_TEST

START_TEST( isdigit )
{
    TESTCASE( isdigit( '0' ) );
    TESTCASE( isdigit( '9' ) );
    TESTCASE( ! isdigit( ' ' ) );
    TESTCASE( ! isdigit( 'a' ) );
    TESTCASE( ! isdigit( '@' ) );
}
END_TEST

START_TEST( isgraph )
{
    TESTCASE( isgraph( 'a' ) );
    TESTCASE( isgraph( 'z' ) );
    TESTCASE( isgraph( 'A' ) );
    TESTCASE( isgraph( 'Z' ) );
    TESTCASE( isgraph( '@' ) );
    TESTCASE( ! isgraph( '\t' ) );
    TESTCASE( ! isgraph( '\0' ) );
    TESTCASE( ! isgraph( ' ' ) );
}
END_TEST

START_TEST( islower )
{
    TESTCASE( islower( 'a' ) );
    TESTCASE( islower( 'z' ) );
    TESTCASE( ! islower( 'A' ) );
    TESTCASE( ! islower( 'Z' ) );
    TESTCASE( ! islower( ' ' ) );
    TESTCASE( ! islower( '@' ) );
}
END_TEST

START_TEST( isprint )
{
    TESTCASE( isprint( 'a' ) );
    TESTCASE( isprint( 'z' ) );
    TESTCASE( isprint( 'A' ) );
    TESTCASE( isprint( 'Z' ) );
    TESTCASE( isprint( '@' ) );
    TESTCASE( ! isprint( '\t' ) );
    TESTCASE( ! isprint( '\0' ) );
    TESTCASE( isprint( ' ' ) );
}
END_TEST

START_TEST( ispunct )
{
    TESTCASE( ! ispunct( 'a' ) );
    TESTCASE( ! ispunct( 'z' ) );
    TESTCASE( ! ispunct( 'A' ) );
    TESTCASE( ! ispunct( 'Z' ) );
    TESTCASE( ispunct( '@' ) );
    TESTCASE( ispunct( '.' ) );
    TESTCASE( ! ispunct( '\t' ) );
    TESTCASE( ! ispunct( '\0' ) );
    TESTCASE( ! ispunct( ' ' ) );
}
END_TEST

START_TEST( isspace )
{
    TESTCASE( isspace( ' ' ) );
    TESTCASE( isspace( '\f' ) );
    TESTCASE( isspace( '\n' ) );
    TESTCASE( isspace( '\r' ) );
    TESTCASE( isspace( '\t' ) );
    TESTCASE( isspace( '\v' ) );
    TESTCASE( ! isspace( 'a' ) );
}
END_TEST

START_TEST( isupper )
{
    TESTCASE( isupper( 'A' ) );
    TESTCASE( isupper( 'Z' ) );
    TESTCASE( ! isupper( 'a' ) );
    TESTCASE( ! isupper( 'z' ) );
    TESTCASE( ! isupper( ' ' ) );
    TESTCASE( ! isupper( '@' ) );
}
END_TEST

START_TEST( isxdigit )
{
    TESTCASE( isxdigit( '0' ) );
    TESTCASE( isxdigit( '9' ) );
    TESTCASE( isxdigit( 'a' ) );
    TESTCASE( isxdigit( 'f' ) );
    TESTCASE( ! isxdigit( 'g' ) );
    TESTCASE( isxdigit( 'A' ) );
    TESTCASE( isxdigit( 'F' ) );
    TESTCASE( ! isxdigit( 'G' ) );
    TESTCASE( ! isxdigit( '@' ) );
    TESTCASE( ! isxdigit( ' ' ) );
}
END_TEST

START_TEST( tolower )
{
    TESTCASE( tolower( 'A' ) == 'a' );
    TESTCASE( tolower( 'Z' ) == 'z' );
    TESTCASE( tolower( 'a' ) == 'a' );
    TESTCASE( tolower( 'z' ) == 'z' );
    TESTCASE( tolower( '@' ) == '@' );
    TESTCASE( tolower( '[' ) == '[' );
}
END_TEST

START_TEST( toupper )
{
    TESTCASE( toupper( 'a' ) == 'A' );
    TESTCASE( toupper( 'z' ) == 'Z' );
    TESTCASE( toupper( 'A' ) == 'A' );
    TESTCASE( toupper( 'Z' ) == 'Z' );
    TESTCASE( toupper( '@' ) == '@' );
    TESTCASE( toupper( '[' ) == '[' );
}
END_TEST


START_SUITE( ctype )
{
	RUN_TEST( isalnum );
	RUN_TEST( isalpha );
	RUN_TEST( isblank );
	RUN_TEST( iscntrl );
	RUN_TEST( isdigit );
	RUN_TEST( isgraph );
	RUN_TEST( islower );
	RUN_TEST( isprint );
	RUN_TEST( ispunct );
	RUN_TEST( isspace );
	RUN_TEST( isupper );
	RUN_TEST( isxdigit );
	RUN_TEST( tolower );
	RUN_TEST( toupper );
}
END_SUITE
