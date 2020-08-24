#include "_PDCLIB_test.h"
#include "_PDCLIB_iotest.h"
#include <assert.h>

START_TEST( atomax )
{
    /* basic functionality */
    TESTCASE( _PDCLIB_atomax( "123" ) == 123 );
    /* testing skipping of leading whitespace and trailing garbage */
    TESTCASE( _PDCLIB_atomax( " \n\v\t\f123xyz" ) == 123 );
}
END_TEST

START_TEST( digits )
{
    TESTCASE( strcmp( _PDCLIB_digits, "0123456789abcdefghijklmnopqrstuvwxyz" ) == 0 );
    TESTCASE( strcmp( _PDCLIB_Xdigits, "0123456789ABCDEF" ) == 0 );
}
END_TEST

START_TEST( filemode )
{
    TESTCASE( _PDCLIB_filemode( "r" ) == _PDCLIB_FREAD );
    TESTCASE( _PDCLIB_filemode( "w" ) == _PDCLIB_FWRITE );
    TESTCASE( _PDCLIB_filemode( "a" ) == ( _PDCLIB_FAPPEND | _PDCLIB_FWRITE ) );
    TESTCASE( _PDCLIB_filemode( "r+" ) == ( _PDCLIB_FREAD | _PDCLIB_FRW ) );
    TESTCASE( _PDCLIB_filemode( "w+" ) == ( _PDCLIB_FWRITE | _PDCLIB_FRW ) );
    TESTCASE( _PDCLIB_filemode( "a+" ) == ( _PDCLIB_FAPPEND | _PDCLIB_FWRITE | _PDCLIB_FRW ) );
    TESTCASE( _PDCLIB_filemode( "rb" ) == ( _PDCLIB_FREAD | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "wb" ) == ( _PDCLIB_FWRITE | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "ab" ) == ( _PDCLIB_FAPPEND | _PDCLIB_FWRITE | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "r+b" ) == ( _PDCLIB_FREAD | _PDCLIB_FRW | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "w+b" ) == ( _PDCLIB_FWRITE | _PDCLIB_FRW | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "a+b" ) == ( _PDCLIB_FAPPEND | _PDCLIB_FWRITE | _PDCLIB_FRW | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "rb+" ) == ( _PDCLIB_FREAD | _PDCLIB_FRW | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "wb+" ) == ( _PDCLIB_FWRITE | _PDCLIB_FRW | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "ab+" ) == ( _PDCLIB_FAPPEND | _PDCLIB_FWRITE | _PDCLIB_FRW | _PDCLIB_FBIN ) );
    TESTCASE( _PDCLIB_filemode( "x" ) == 0 );
    TESTCASE( _PDCLIB_filemode( "r++" ) == 0 );
    TESTCASE( _PDCLIB_filemode( "wbb" ) == 0 );
    TESTCASE( _PDCLIB_filemode( "a+bx" ) == 0 );
}
END_TEST

START_TEST( is_leap )
{
    /* 1901 not leap */
    TESTCASE( ! _PDCLIB_is_leap( 1 ) );
    /* 1904 leap */
    TESTCASE( _PDCLIB_is_leap( 4 ) );
    /* 1900 not leap */
    TESTCASE( ! _PDCLIB_is_leap( 0 ) );
    /* 2000 leap */
    TESTCASE( _PDCLIB_is_leap( 100 ) );
}
END_TEST

START_TEST( load_lc_ctype )
{
    FILE * fh = fopen( "test_ctype.dat", "wb" );
    TESTCASE_REQUIRE( fh != NULL );
    /* For test purposes, let's set up a charset that only has the hex digits */
    /* 0x00..0x09 - digits */
    /* 0x11..0x16 - Xdigits */
    /* 0x21..0x26 - xdigits */
    TESTCASE( j6libc_fprintf( fh, "%x %x\n", 0x00, 0x09 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x %x\n", 0x11, 0x16, 0x21, 0x26 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x00, 0x00 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x01, 0x01 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x02, 0x02 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x03, 0x03 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x04, 0x04 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x05, 0x05 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x06, 0x06 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x07, 0x07 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x08, 0x08 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH, 0x09, 0x09 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x0A, 0x0A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x0B, 0x0B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x0C, 0x0C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x0D, 0x0D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x0E, 0x0E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x0F, 0x0F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x10, 0x10 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_UPPER, 0x11, 0x11 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_UPPER, 0x12, 0x12 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_UPPER, 0x13, 0x13 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_UPPER, 0x14, 0x14 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_UPPER, 0x15, 0x15 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_UPPER, 0x16, 0x16 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x17, 0x17 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x18, 0x18 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x19, 0x19 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x1A, 0x1A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x1B, 0x1B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x1C, 0x1C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x1D, 0x1D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x1E, 0x1E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x1F, 0x1F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x20, 0x20 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_LOWER, 0x21, 0x21 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_LOWER, 0x22, 0x22 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_LOWER, 0x23, 0x23 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_LOWER, 0x24, 0x24 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_LOWER, 0x25, 0x25 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", _PDCLIB_CTYPE_GRAPH | _PDCLIB_CTYPE_ALPHA | _PDCLIB_CTYPE_LOWER, 0x26, 0x26 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x27, 0x27 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x28, 0x28 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x29, 0x29 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x2A, 0x2A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x2B, 0x2B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x2C, 0x2C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x2D, 0x2D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x2E, 0x2E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x2F, 0x2F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x30, 0x30 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x31, 0x31 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x32, 0x32 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x33, 0x33 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x34, 0x34 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x35, 0x35 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x36, 0x36 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x37, 0x37 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x38, 0x38 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x39, 0x39 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x3A, 0x3A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x3B, 0x3B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x3C, 0x3C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x3D, 0x3D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x3E, 0x3E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x3F, 0x3F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x40, 0x40 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x41, 0x41 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x42, 0x42 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x43, 0x43 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x44, 0x44 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x45, 0x45 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x46, 0x46 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x47, 0x47 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x48, 0x48 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x49, 0x49 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x4A, 0x4A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x4B, 0x4B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x4C, 0x4C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x4D, 0x4D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x4E, 0x4E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x4F, 0x4F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x50, 0x50 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x51, 0x51 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x52, 0x52 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x53, 0x53 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x54, 0x54 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x55, 0x55 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x56, 0x56 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x57, 0x57 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x58, 0x58 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x59, 0x59 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x5A, 0x5A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x5B, 0x5B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x5C, 0x5C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x5D, 0x5D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x5E, 0x5E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x5F, 0x5F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x60, 0x60 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x61, 0x61 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x62, 0x62 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x63, 0x63 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x64, 0x64 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x65, 0x65 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x66, 0x66 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x67, 0x67 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x68, 0x68 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x69, 0x69 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x6A, 0x6A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x6B, 0x6B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x6C, 0x6C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x6D, 0x6D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x6E, 0x6E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x6F, 0x6F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x70, 0x70 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x71, 0x71 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x72, 0x72 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x73, 0x73 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x74, 0x74 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x75, 0x75 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x76, 0x76 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x77, 0x77 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x78, 0x78 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x79, 0x79 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x7A, 0x7A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x7B, 0x7B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x7C, 0x7C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x7D, 0x7D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x7E, 0x7E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x7F, 0x7F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x80, 0x80 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x81, 0x81 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x82, 0x82 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x83, 0x83 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x84, 0x84 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x85, 0x85 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x86, 0x86 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x87, 0x87 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x88, 0x88 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x89, 0x89 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x8A, 0x8A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x8B, 0x8B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x8C, 0x8C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x8D, 0x8D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x8E, 0x8E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x8F, 0x8F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x90, 0x90 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x91, 0x91 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x92, 0x92 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x93, 0x93 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x94, 0x94 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x95, 0x95 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x96, 0x96 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x97, 0x97 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x98, 0x98 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x99, 0x99 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x9A, 0x9A ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x9B, 0x9B ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x9C, 0x9C ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x9D, 0x9D ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x9E, 0x9E ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0x9F, 0x9F ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA0, 0xA0 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA1, 0xA1 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA2, 0xA2 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA3, 0xA3 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA4, 0xA4 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA5, 0xA5 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA6, 0xA6 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA7, 0xA7 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA8, 0xA8 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xA9, 0xA9 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xAA, 0xAA ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xAB, 0xAB ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xAC, 0xAC ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xAD, 0xAD ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xAE, 0xAE ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xAF, 0xAF ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB0, 0xB0 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB1, 0xB1 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB2, 0xB2 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB3, 0xB3 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB4, 0xB4 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB5, 0xB5 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB6, 0xB6 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB7, 0xB7 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB8, 0xB8 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xB9, 0xB9 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xBA, 0xBA ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xBB, 0xBB ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xBC, 0xBC ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xBD, 0xBD ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xBE, 0xBE ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xBF, 0xBF ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC0, 0xC0 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC1, 0xC1 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC2, 0xC2 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC3, 0xC3 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC4, 0xC4 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC5, 0xC5 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC6, 0xC6 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC7, 0xC7 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC8, 0xC8 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xC9, 0xC9 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xCA, 0xCA ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xCB, 0xCB ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xCC, 0xCC ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xCD, 0xCD ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xCE, 0xCE ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xCF, 0xCF ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD0, 0xD0 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD1, 0xD1 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD2, 0xD2 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD3, 0xD3 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD4, 0xD4 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD5, 0xD5 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD6, 0xD6 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD7, 0xD7 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD8, 0xD8 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xD9, 0xD9 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xDA, 0xDA ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xDB, 0xDB ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xDC, 0xDC ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xDD, 0xDD ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xDE, 0xDE ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xDF, 0xDF ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE0, 0xE0 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE1, 0xE1 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE2, 0xE2 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE3, 0xE3 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE4, 0xE4 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE5, 0xE5 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE6, 0xE6 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE7, 0xE7 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE8, 0xE8 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xE9, 0xE9 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xEA, 0xEA ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xEB, 0xEB ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xEC, 0xEC ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xED, 0xED ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xEE, 0xEE ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xEF, 0xEF ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF0, 0xF0 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF1, 0xF1 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF2, 0xF2 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF3, 0xF3 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF4, 0xF4 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF5, 0xF5 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF6, 0xF6 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF7, 0xF7 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF8, 0xF8 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xF9, 0xF9 ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xFA, 0xFA ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xFB, 0xFB ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xFC, 0xFC ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xFD, 0xFD ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xFE, 0xFE ) );
    TESTCASE( j6libc_fprintf( fh, "%x %x %x\n", 0x00, 0xFF, 0xFF) );
    fclose( fh );
    TESTCASE( _PDCLIB_load_lc_ctype( "./", "test" ) != NULL );
    remove( "test_ctype.dat" );
    /*
    TESTCASE( isdigit( 0x00 ) && ! isxdigit( 0x00 ) && ! isalpha( 0x00 ) );
    TESTCASE( ! isdigit( 0x11 ) && isxdigit( 0x11 ) && isalpha( 0x11 ) && isupper( 0x11 ) && ! islower( 0x11 ) );
    TESTCASE( ! isdigit( 0x21 ) && isxdigit( 0x21 ) && isalpha( 0x21 ) && ! isupper( 0x11 ) && islower( 0x11 ) );
    */
}
END_TEST

START_TEST( load_lc_messages )
{
    FILE * fh = fopen( "test_numeric.dat", "wb" );
    struct _PDCLIB_lc_lconv_numeric_t * lc;

    TESTCASE_REQUIRE( fh != NULL );

    TESTCASE( fputs( ",\n.\n\n", fh ) != EOF );
    fclose( fh );
    TESTCASE( ( lc = _PDCLIB_load_lc_numeric( "./", "test" ) ) );
    remove( "test_numeric.dat" );
    TESTCASE( strcmp( lc->decimal_point, "," ) == 0 );
    TESTCASE( strcmp( lc->thousands_sep, "." ) == 0 );
    TESTCASE( strcmp( lc->grouping, "" ) == 0 );
}
END_TEST

START_TEST( load_lc_monetary )
{
    FILE * fh = fopen( "test_monetary.dat", "wb" );
    struct _PDCLIB_lc_lconv_monetary_t * lc;
    TESTCASE_REQUIRE( fh != NULL );

    j6libc_fprintf( fh, "%s\n", "," );    /* mon_decimal_point */
    j6libc_fprintf( fh, "%s\n", "." );    /* mon_thousands_sep */
    j6libc_fprintf( fh, "%s\n", "3" );    /* mon_grouping */
    j6libc_fprintf( fh, "%s\n", "" );     /* positive_sign */
    j6libc_fprintf( fh, "%s\n", "-" );    /* negative_sign */
    j6libc_fprintf( fh, "%s\n", "\xa4" ); /* currency_symbol */
    j6libc_fprintf( fh, "%s\n", "EUR" );  /* int_curr_symbol */
    fputc( 2, fh ); /* frac_digits */
    fputc( 0, fh ); /* p_cs_precedes */
    fputc( 0, fh ); /* n_cs_precedes */
    fputc( 1, fh ); /* p_sep_by_space */
    fputc( 1, fh ); /* n_sep_by_space */
    fputc( 1, fh ); /* p_sign_posn */
    fputc( 1, fh ); /* n_sign_posn */
    fputc( 2, fh ); /* int_frac_digits */
    fputc( 0, fh ); /* int_p_cs_precedes */
    fputc( 0, fh ); /* int_n_cs_precedes */
    fputc( 1, fh ); /* int_p_sep_by_space */
    fputc( 1, fh ); /* int_n_sep_by_space */
    fputc( 1, fh ); /* int_p_sign_posn */
    fputc( 1, fh ); /* int_n_sign_posn */
    j6libc_fprintf( fh, "\n" );
    fclose( fh );
    TESTCASE( ( lc = _PDCLIB_load_lc_monetary( "./", "test" ) ) );
    remove( "test_monetary.dat" );
    TESTCASE( strcmp( lc->mon_decimal_point, "," ) == 0 );
    TESTCASE( strcmp( lc->mon_thousands_sep, "." ) == 0 );
    TESTCASE( strcmp( lc->mon_grouping, "3" ) == 0 );
    TESTCASE( strcmp( lc->positive_sign, "" ) == 0 );
    TESTCASE( strcmp( lc->negative_sign, "-" ) == 0 );
    TESTCASE( strcmp( lc->currency_symbol, "\xa4" ) == 0 );
    TESTCASE( strcmp( lc->int_curr_symbol, "EUR" ) == 0 );

    TESTCASE( lc->frac_digits == 2 );
    TESTCASE( lc->p_cs_precedes == 0 );
    TESTCASE( lc->n_cs_precedes == 0 );
    TESTCASE( lc->p_sep_by_space == 1 );
    TESTCASE( lc->n_sep_by_space == 1 );
    TESTCASE( lc->p_sign_posn == 1 );
    TESTCASE( lc->n_sign_posn == 1 );
    TESTCASE( lc->int_frac_digits == 2 );
    TESTCASE( lc->int_p_cs_precedes == 0 );
    TESTCASE( lc->int_n_cs_precedes == 0 );
    TESTCASE( lc->int_p_sep_by_space == 1 );
    TESTCASE( lc->int_n_sep_by_space == 1 );
    TESTCASE( lc->int_p_sign_posn == 1 );
    TESTCASE( lc->int_n_sign_posn == 1 );
}
END_TEST

START_TEST( load_lc_numeric )
{
    FILE * fh = fopen( "test_numeric.dat", "wb" );
    struct _PDCLIB_lc_lconv_numeric_t * lc;
    TESTCASE_REQUIRE( fh != NULL );
    TESTCASE( fputs( ",\n.\n\n", fh ) != EOF );
    fclose( fh );
    TESTCASE( ( lc = _PDCLIB_load_lc_numeric( "./", "test" ) ) );
    remove( "test_numeric.dat" );
    TESTCASE( strcmp( lc->decimal_point, "," ) == 0 );
    TESTCASE( strcmp( lc->thousands_sep, "." ) == 0 );
    TESTCASE( strcmp( lc->grouping, "" ) == 0 );
}
END_TEST

START_TEST( load_lc_time )
{
    FILE * fh = fopen( "test_time.dat", "wb" );
    struct _PDCLIB_lc_time_t * lc;

    TESTCASE_REQUIRE( fh != NULL );

    /* month name abbreviation */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Jan" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Feb" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "M\xe4r" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Apr" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Mai" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Jun" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Jul" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Aug" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Sep" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Okt" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Nov" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Dez" ) == 4 );
    /* month name full */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Januar" ) == 7 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Februar" ) == 8 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "M\xe4rz" ) == 5 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "April" ) == 6 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Mai" ) == 4 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Juni" ) == 5 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Juli" ) == 5 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "August" ) == 7 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "September" ) == 10 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Oktober" ) == 8 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "November" ) == 9 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Dezember" ) == 9 );
    /* day name abbreviation */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "So" ) == 3 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Mo" ) == 3 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Di" ) == 3 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Mi" ) == 3 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Do" ) == 3 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Fr" ) == 3 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Sa" ) == 3 );
    /* day name full */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Sonntag" ) == 8 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Montag" ) == 7 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Dienstag" ) == 9 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Mittwoch" ) == 9 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Donnerstag" ) == 11 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Freitag" ) == 8 );
    TESTCASE( j6libc_fprintf( fh, "%s\n", "Samstag" ) == 8 );

    TESTCASE( j6libc_fprintf( fh, "%s\n", "%a %d %b %Y %T %Z" ) == 18 ); /* date time format (%c) */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "%I:%M:%S" ) == 9 ); /* 12-hour time format (%r) */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "%d.%m.%Y" ) == 9 ); /* date format (%x) */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "%H:%M:%S" ) == 9 ); /* time format (%X) */

    TESTCASE( j6libc_fprintf( fh, "%s\n", "" ) == 1 ); /* AM */
    TESTCASE( j6libc_fprintf( fh, "%s\n", "" ) == 1 ); /* PM */
    fclose( fh );
    TESTCASE( ( lc = _PDCLIB_load_lc_time( "./", "test" ) ) );
    remove( "test_time.dat" );

    TESTCASE( strcmp( lc->month_name_abbr[ 0 ], "Jan" ) == 0 );
    TESTCASE( strcmp( lc->month_name_abbr[ 11 ], "Dez" ) == 0 );
    TESTCASE( strcmp( lc->month_name_full[ 0 ], "Januar" ) == 0 );
    TESTCASE( strcmp( lc->month_name_full[ 11 ], "Dezember" ) == 0 );
    TESTCASE( strcmp( lc->day_name_abbr[ 0 ], "So" ) == 0 );
    TESTCASE( strcmp( lc->day_name_abbr[ 6 ], "Sa" ) == 0 );
    TESTCASE( strcmp( lc->day_name_full[ 0 ], "Sonntag" ) == 0 );
    TESTCASE( strcmp( lc->day_name_full[ 6 ], "Samstag" ) == 0 );
}
END_TEST

START_TEST( load_lines )
{
    FILE * fh = fopen( "test_lines.txt", "w+" );
    char * rc;

    TESTCASE_REQUIRE( fh != NULL );
    TESTCASE( fputs( "Foo\n\nBar\n", fh ) != EOF );

    rewind( fh );
    rc = _PDCLIB_load_lines( fh, 3 );
    fclose( fh );
    remove( "test_lines.txt" );

    TESTCASE( rc != NULL );
    TESTCASE( strcmp( rc, "Foo" ) == 0 );
    TESTCASE( strcmp( rc + 4, "" ) == 0 );
    TESTCASE( strcmp( rc + 5, "Bar" ) == 0 );
}
END_TEST

static int testprintf(
		const char *implfile,
		const char *file,
		int line,
		size_t expected_rc,
		const char *expected_out,
		const char *format, 
		... )
{
    char buffer[100];

    /* Members: base, flags, n, i, current, s, width, prec, stream, arg      */
    struct _PDCLIB_status_t status;
    status.base = 0;
    status.flags = 0;
    status.n = 100;
    status.i = 0;
    status.current = 0;
    status.s = buffer;
    status.width = 0;
    status.prec = EOF;
    status.stream = NULL;

    va_start( status.arg, format );
    memset( buffer, '\0', 100 );
    if ( *(_PDCLIB_print( format, &status )) != '\0' )
    {
        fprintf( stderr, "_PDCLIB_print() did not return end-of-specifier on '%s'.\n", format );
		return 1;
    }
    va_end( status.arg );

    if (status.i != expected_rc ||
		strcmp(buffer, expected_out))
	{
		fprintf( stderr,
			"FAILED: %s (%s), line %d\n"
			"        format string \"%s\"\n"
			"        expected %2lu, \"%s\"\n"
			"        actual   %2lu, \"%s\"\n",
			 file, implfile, line, format, expected_rc,
			 expected_out, status.i, buffer );
		return 1;
	}

	return 0;
}

START_TEST( print )
{
#define IMPLFILE "_PDCLIB/print.c"
#define DO_TESTPRINTF testprintf
#define TEST_CONVERSION_ONLY
#include "printf_testcases.h"
#undef TEST_CONVERSION_ONLY
#undef DO_TESTPRINTF
#undef IMPLFILE
}
END_TEST

static int testscanf(
		const char *implfile,
		const char *file,
		int line,
		size_t expected_rc,
		const char *input_string,
		const char *format, 
		... )
{
    char buffer[100];
	memcpy( buffer, input_string, strlen(input_string)+1 );

    struct _PDCLIB_status_t status;
    status.n = 0;
    status.i = 0;
    status.s = (char *)buffer;
    status.stream = NULL;

    va_start( status.arg, format );
    if ( *(_PDCLIB_scan( format, &status )) != '\0' )
    {
        fprintf( stderr, "_PDCLIB_scan() did not return end-of-specifier on '%s'.\n", format );
		return 1;
    }
    va_end( status.arg );

	if ( status.n != expected_rc )
	{
		fprintf( stderr,
				"FAILED: %s (%s), line %d\n"
				"        expected %2lu,\n"
				"        actual   %2lu\n",
				file, implfile, line, expected_rc, status.n );
		return 1;
	}

    return 0;
}

START_TEST( scan )
{
#define IMPLFILE "_PDCLIB/scan.c"
#define DO_TESTSCANF testscanf
#define TEST_CONVERSION_ONLY
#include "scanf_testcases.h"
#undef TEST_CONVERSION_ONLY
#undef DO_TESTSCANF
#undef IMPLFILE
}
END_TEST

START_TEST( strtox_main )
{
    const char * p;
    char test[] = "123_";
    char fail[] = "xxx";
    char sign = '-';
    errno = 0;
    p = test;

    /* basic functionality */
    TESTCASE(
        (_PDCLIB_strtox_main(&p, 10u, (uintmax_t)999, (uintmax_t)12, 3, &sign) == 123) &&
        (errno == 0) &&
        (p == &test[3]) );

    /* proper functioning to smaller base */
    TESTCASE(
        (_PDCLIB_strtox_main(&p, 8u, (uintmax_t)999, (uintmax_t)12, 3, &sign) == 0123) &&
        (errno == 0) &&
        (p == &test[3]) );

    /* overflowing subject sequence must still return proper endptr */
    TESTCASE(
        (_PDCLIB_strtox_main(&p, 4u, (uintmax_t)999, (uintmax_t)1, 2, &sign) == 999) &&
        (errno == ERANGE) &&
        (p == &test[3]) &&
        (sign == '+') );

    /* testing conversion failure */
    p = fail;
    sign = '-';
    TESTCASE(
        (_PDCLIB_strtox_main(&p, 10u, (uintmax_t)999, (uintmax_t)99, 8, &sign) == 0) &&
        (p == NULL) );
}
END_TEST

START_TEST( strtox_prelim )
{
    int base = 0;
    char sign = '\0';
    char test1[] = "  123";
    char test2[] = "\t+0123";
    char test3[] = "\v-0x123";

    TESTCASE(
        (_PDCLIB_strtox_prelim(test1, &sign, &base) == &test1[2]) &&
        (sign == '+') &&
        (base == 10) );

    TESTCASE(
        (_PDCLIB_strtox_prelim(test2, &sign, &base) == &test2[3]) &&
        (sign == '+') &&
        (base == 8) );

    TESTCASE(
        (_PDCLIB_strtox_prelim(test3, &sign, &base) == &test3[4]) &&
        (sign == '-') &&
        (base == 16) );

    base = 10;
    TESTCASE(
        (_PDCLIB_strtox_prelim(test3, &sign, &base) == &test3[2]) &&
        (sign == '-') &&
        (base == 10) );

    base = 1;
    TESTCASE( _PDCLIB_strtox_prelim(test3, &sign, &base) == NULL );

    base = 37;
    TESTCASE( _PDCLIB_strtox_prelim(test3, &sign, &base) == NULL );
}
END_TEST



START_SUITE( internal )
{
	RUN_TEST( atomax );
	RUN_TEST( digits );
	RUN_TEST( filemode );
	RUN_TEST( is_leap );
	RUN_TEST( print );
	RUN_TEST( scan );
	RUN_TEST( strtox_main );
	RUN_TEST( strtox_prelim );


	// These fail due to lack of open()
	/*
	RUN_TEST( load_lc_ctype );
	RUN_TEST( load_lc_messages );
	RUN_TEST( load_lc_monetary );
	RUN_TEST( load_lc_numeric );
	RUN_TEST( load_lc_time );
	RUN_TEST( load_lines );
	*/
}
END_SUITE
