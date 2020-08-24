#include "_PDCLIB_test.h"

START_TEST( memchr )
{
    TESTCASE( memchr( abcde, 'c', 5 ) == &abcde[2] );
    TESTCASE( memchr( abcde, 'a', 1 ) == &abcde[0] );
    TESTCASE( memchr( abcde, 'a', 0 ) == NULL );
    TESTCASE( memchr( abcde, '\0', 5 ) == NULL );
    TESTCASE( memchr( abcde, '\0', 6 ) == &abcde[5] );
}
END_TEST

START_TEST( memcmp )
{
    const char xxxxx[] = "xxxxx";
    TESTCASE( memcmp( abcde, abcdx, 5 ) < 0 );
    TESTCASE( memcmp( abcde, abcdx, 4 ) == 0 );
    TESTCASE( memcmp( abcde, xxxxx, 0 ) == 0 );
    TESTCASE( memcmp( xxxxx, abcde, 1 ) > 0 );
}
END_TEST

START_TEST( memcpy )
{
    char s[] = "xxxxxxxxxxx";
    char *r = memcpy(s, abcde, 6);
    TESTCASE( r == s );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == '\0' );

    r = memcpy(s + 5, abcde, 5);
    TESTCASE( r == s + 5 );
    TESTCASE( s[9] == 'e' );
    TESTCASE( s[10] == 'x' );
}
END_TEST

START_TEST( memmove )
{
    char s[] = "xxxxabcde";
    char *r = memmove(s, s + 4, 5);
    TESTCASE( r == s );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == 'b' );

    r = memmove(s + 4, s, 5);
    TESTCASE( r == s + 4 );
    TESTCASE( s[4] == 'a' );
}
END_TEST

START_TEST( memset )
{
    char s[] = "xxxxxxxxx";
    void *r = memset(s, 'o', 10);
    TESTCASE( r == s );
    TESTCASE( s[9] == 'o' );

    r = memset(s, '_', 0);
    TESTCASE( r == s );
    TESTCASE( s[0] == 'o' );

    r = memset(s, '_', 1);
    TESTCASE( r == s );
    TESTCASE( s[0] == '_' );
    TESTCASE( s[1] == 'o' );
}
END_TEST

START_TEST( strcat )
{
    char s[] = "xx\0xxxxxx";
    char *r = strcat(s, abcde);
    TESTCASE( r == s );
    TESTCASE( s[2] == 'a' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( s[7] == '\0' );
    TESTCASE( s[8] == 'x' );

    s[0] = '\0';
    r = strcat(s, abcdx);
    TESTCASE( r == s );
    TESTCASE( s[4] == 'x' );
    TESTCASE( s[5] == '\0' );

    r = strcat(s, "\0");
    TESTCASE( r == s );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );
}
END_TEST

START_TEST( strchr )
{
    char abccd[] = "abccd";
    TESTCASE( strchr( abccd, 'x' ) == NULL );
    TESTCASE( strchr( abccd, 'a' ) == &abccd[0] );
    TESTCASE( strchr( abccd, 'd' ) == &abccd[4] );
    TESTCASE( strchr( abccd, '\0' ) == &abccd[5] );
    TESTCASE( strchr( abccd, 'c' ) == &abccd[2] );
}
END_TEST

START_TEST( strcmp )
{
    char cmpabcde[] = "abcde";
    char cmpabcd_[] = "abcd\xfc";
    char empty[] = "";
    TESTCASE( strcmp( abcde, cmpabcde ) == 0 );
    TESTCASE( strcmp( abcde, abcdx ) < 0 );
    TESTCASE( strcmp( abcdx, abcde ) > 0 );
    TESTCASE( strcmp( empty, abcde ) < 0 );
    TESTCASE( strcmp( abcde, empty ) > 0 );
    TESTCASE( strcmp( abcde, cmpabcd_ ) < 0 );
}
END_TEST

START_TEST( strcoll )
{
    char cmpabcde[] = "abcde";
    char empty[] = "";
    TESTCASE( strcoll( abcde, cmpabcde ) == 0 );
    TESTCASE( strcoll( abcde, abcdx ) < 0 );
    TESTCASE( strcoll( abcdx, abcde ) > 0 );
    TESTCASE( strcoll( empty, abcde ) < 0 );
    TESTCASE( strcoll( abcde, empty ) > 0 );
}
END_TEST

START_TEST( strcpy )
{
    char s[] = "xxxxx";
    char *r = strcpy(s, "");
    TESTCASE( r == s );
    TESTCASE( s[0] == '\0' );
    TESTCASE( s[1] == 'x' );

    r = strcpy(s, abcde);
    TESTCASE( r == s );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == '\0' );
}
END_TEST

START_TEST( strcspn )
{
    TESTCASE( strcspn( abcde, "x" ) == 5 );
    TESTCASE( strcspn( abcde, "xyz" ) == 5 );
    TESTCASE( strcspn( abcde, "zyx" ) == 5 );
    TESTCASE( strcspn( abcdx, "x" ) == 4 );
    TESTCASE( strcspn( abcdx, "xyz" ) == 4 );
    TESTCASE( strcspn( abcdx, "zyx" ) == 4 );
    TESTCASE( strcspn( abcde, "a" ) == 0 );
    TESTCASE( strcspn( abcde, "abc" ) == 0 );
    TESTCASE( strcspn( abcde, "cba" ) == 0 );
}
END_TEST

START_TEST( strerror )
{
    TESTCASE( strerror(ERANGE) != strerror(EDOM) );
}
END_TEST

START_TEST( strlen )
{
    TESTCASE( strlen( abcde ) == 5 );
    TESTCASE( strlen( "" ) == 0 );
}
END_TEST

START_TEST( strncat )
{
    char s[] = "xx\0xxxxxx";
    char *r = strncat(s, abcde, 10);
    TESTCASE( r == s );
    TESTCASE( s[2] == 'a' );
    TESTCASE( s[6] == 'e' );
    TESTCASE( s[7] == '\0' );
    TESTCASE( s[8] == 'x' );

    s[0] = '\0';
    r = strncat(s, abcdx, 10);
    TESTCASE( r == s );
    TESTCASE( s[4] == 'x' );
    TESTCASE( s[5] == '\0' );

    r = strncat(s, "\0", 10);
    TESTCASE( r == s );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );

    r = strncat(s, abcde, 0);
    TESTCASE( r == s );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'e' );

    r = strncat(s, abcde, 3);
    TESTCASE( r == s );
    TESTCASE( s[5] == 'a' );
    TESTCASE( s[7] == 'c' );
    TESTCASE( s[8] == '\0' );
}
END_TEST

START_TEST( strncmp )
{
    char cmpabcde[] = "abcde\0f";
    char cmpabcd_[] = "abcde\xfc";
    char empty[] = "";
    char x[] = "x";
    TESTCASE( strncmp( abcde, cmpabcde, 5 ) == 0 );
    TESTCASE( strncmp( abcde, cmpabcde, 10 ) == 0 );
    TESTCASE( strncmp( abcde, abcdx, 5 ) < 0 );
    TESTCASE( strncmp( abcdx, abcde, 5 ) > 0 );
    TESTCASE( strncmp( empty, abcde, 5 ) < 0 );
    TESTCASE( strncmp( abcde, empty, 5 ) > 0 );
    TESTCASE( strncmp( abcde, abcdx, 4 ) == 0 );
    TESTCASE( strncmp( abcde, x, 0 ) == 0 );
    TESTCASE( strncmp( abcde, x, 1 ) < 0 );
    TESTCASE( strncmp( abcde, cmpabcd_, 10 ) < 0 );
}
END_TEST

START_TEST( strncpy )
{
    char s[] = "xxxxxxx";
    char *r = strncpy(s, "", 1);
    TESTCASE( r == s );
    TESTCASE( s[0] == '\0' );
    TESTCASE( s[1] == 'x' );

    r = strncpy(s, abcde, 6);
    TESTCASE( r == s );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[4] == 'e' );
    TESTCASE( s[5] == '\0' );
    TESTCASE( s[6] == 'x' );

    r = strncpy(s, abcde, 7);
    TESTCASE( r == s );
    TESTCASE( s[6] == '\0' );

    r = strncpy(s, "xxxx", 3);
    TESTCASE( r == s );
    TESTCASE( s[0] == 'x' );
    TESTCASE( s[2] == 'x' );
    TESTCASE( s[3] == 'd' );
}
END_TEST

START_TEST( strpbrk )
{
    TESTCASE( strpbrk( abcde, "x" ) == NULL );
    TESTCASE( strpbrk( abcde, "xyz" ) == NULL );
    TESTCASE( strpbrk( abcdx, "x" ) == &abcdx[4] );
    TESTCASE( strpbrk( abcdx, "xyz" ) == &abcdx[4] );
    TESTCASE( strpbrk( abcdx, "zyx" ) == &abcdx[4] );
    TESTCASE( strpbrk( abcde, "a" ) == &abcde[0] );
    TESTCASE( strpbrk( abcde, "abc" ) == &abcde[0] );
    TESTCASE( strpbrk( abcde, "cba" ) == &abcde[0] );
}
END_TEST

START_TEST( strrchr )
{
    char abccd[] = "abccd";
    TESTCASE( strrchr( abcde, '\0' ) == &abcde[5] );
    TESTCASE( strrchr( abcde, 'e' ) == &abcde[4] );
    TESTCASE( strrchr( abcde, 'a' ) == &abcde[0] );
    TESTCASE( strrchr( abccd, 'c' ) == &abccd[3] );
}
END_TEST

START_TEST( strspn )
{
    TESTCASE( strspn( abcde, "abc" ) == 3 );
    TESTCASE( strspn( abcde, "b" ) == 0 );
    TESTCASE( strspn( abcde, abcde ) == 5 );
}
END_TEST

START_TEST( strstr )
{
    char s[] = "abcabcabcdabcde";
    TESTCASE( strstr( s, "x" ) == NULL );
    TESTCASE( strstr( s, "xyz" ) == NULL );
    TESTCASE( strstr( s, "a" ) == &s[0] );
    TESTCASE( strstr( s, "abc" ) == &s[0] );
    TESTCASE( strstr( s, "abcd" ) == &s[6] );
    TESTCASE( strstr( s, "abcde" ) == &s[10] );
}
END_TEST

START_TEST( strtok )
{
    char s[] = "_a_bc__d_";
    char *r = strtok(s, "_");
    TESTCASE( r == &s[1] );
    TESTCASE( s[1] == 'a' );
    TESTCASE( s[2] == '\0' );

    r = strtok(NULL, "_");
    TESTCASE( r == &s[3] );
    TESTCASE( s[3] == 'b' );
    TESTCASE( s[4] == 'c' );
    TESTCASE( s[5] == '\0' );

    r = strtok(NULL, "_");
    TESTCASE( r == &s[7] );
    TESTCASE( s[6] == '_' );
    TESTCASE( s[7] == 'd' );
    TESTCASE( s[8] == '\0' );

    r = strtok(NULL, "_");
    TESTCASE( r == NULL );

    strcpy( s, "ab_cd" );
    r = strtok(s, "_");
    TESTCASE( r == &s[0] );
    TESTCASE( s[0] == 'a' );
    TESTCASE( s[1] == 'b' );
    TESTCASE( s[2] == '\0' );

    r = strtok(NULL, "_");
    TESTCASE( r == &s[3] );
    TESTCASE( s[3] == 'c' );
    TESTCASE( s[4] == 'd' );
    TESTCASE( s[5] == '\0' );

    r = strtok(NULL, "_");
    TESTCASE( r == NULL );
}
END_TEST

START_TEST( strxfrm )
{
    char s[] = "xxxxxxxxxxx";
    size_t r = strxfrm(NULL, "123456789012", 0);
    TESTCASE( r == 12 );

    r = strxfrm(s, "123456789012", 12);
    TESTCASE( r == 12 );

    /*
    The following test case is true in *this* implementation, but doesn't have to.
    TESTCASE( s[0] == 'x' );
    */

    r = strxfrm(s, "1234567890", 11);
    TESTCASE( r == 10 );
    TESTCASE( s[0] == '1' );
    TESTCASE( s[9] == '0' );
    TESTCASE( s[10] == '\0' );
}
END_TEST


START_SUITE( string )
{
    RUN_TEST( memchr );
    RUN_TEST( memcmp );
    RUN_TEST( memcpy );
    RUN_TEST( memmove );
    RUN_TEST( memset );
    RUN_TEST( strcat );
    RUN_TEST( strchr );
    RUN_TEST( strcmp );
    RUN_TEST( strcoll );
    RUN_TEST( strcpy );
    RUN_TEST( strcspn );
    RUN_TEST( strerror );
    RUN_TEST( strlen );
    RUN_TEST( strncat );
    RUN_TEST( strncmp );
    RUN_TEST( strncpy );
    RUN_TEST( strpbrk );
    RUN_TEST( strrchr );
    RUN_TEST( strspn );
    RUN_TEST( strstr );
    RUN_TEST( strtok );
    RUN_TEST( strxfrm );
}
END_SUITE
