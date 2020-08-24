#include "_PDCLIB_test.h"
#include "_PDCLIB_iotest.h"
#include <assert.h>

extern struct _PDCLIB_file_t *_PDCLIB_filelist;

START_TEST( clearerr )
{
    FILE * fh = tmpfile();

    TESTCASE_REQUIRE( fh != NULL );

    /* Flags should be clear */
    TESTCASE( ! ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    /* Reading from empty stream - should provoke EOF */
    rewind( fh );
    TESTCASE( fgetc( fh ) == EOF );
    TESTCASE( ! ferror( fh ) );
    TESTCASE( feof( fh ) );
    /* clearerr() should clear flags */
    clearerr( fh );
    TESTCASE( ! ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    /* reopen() the file write-only */
    TESTCASE( ( fh = freopen( NULL, "w", fh ) ) != NULL );
    /* Reading from write-only stream - should provoke error */
    TESTCASE( fgetc( fh ) == EOF );
    TESTCASE( ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    /* clearerr() should clear flags */
    clearerr( fh );
    TESTCASE( ! ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    TESTCASE( fclose( fh ) == 0 );
}
END_TEST

START_TEST( fclose )
{
    struct _PDCLIB_file_t * file1 = NULL;
    struct _PDCLIB_file_t * file2 = NULL;
    remove( testfile1 );
    remove( testfile2 );
    TESTCASE( _PDCLIB_filelist == stdin );
    TESTCASE( ( file1 = fopen( testfile1, "w" ) ) != NULL );
    TESTCASE( _PDCLIB_filelist == file1 );
    TESTCASE( ( file2 = fopen( testfile2, "w" ) ) != NULL );
    TESTCASE( _PDCLIB_filelist == file2 );
    TESTCASE( fclose( file2 ) == 0 );
    TESTCASE( _PDCLIB_filelist == file1 );
    TESTCASE( ( file2 = fopen( testfile2, "w" ) ) != NULL );
    TESTCASE( _PDCLIB_filelist == file2 );
    TESTCASE( fclose( file1 ) == 0 );
    TESTCASE( _PDCLIB_filelist == file2 );
    TESTCASE( fclose( file2 ) == 0 );
    TESTCASE( _PDCLIB_filelist == stdin );
    TESTCASE( remove( testfile1 ) == 0 );
    TESTCASE( remove( testfile2 ) == 0 );
}
END_TEST

START_TEST( fgetpos )
{
    FILE * fh = tmpfile();
    fpos_t pos1, pos2;
    TESTCASE_REQUIRE( fh != NULL );
    TESTCASE( fgetpos( fh, &pos1 ) == 0 );
    TESTCASE( fwrite( teststring, 1, strlen( teststring ), fh ) == strlen( teststring ) );
    TESTCASE( fgetpos( fh, &pos2 ) == 0 );
    TESTCASE( fsetpos( fh, &pos1 ) == 0 );
    TESTCASE( ftell( fh ) == 0 );
    TESTCASE( fsetpos( fh, &pos2 ) == 0 );
    TESTCASE( (size_t)ftell( fh ) == strlen( teststring ) );
    TESTCASE( fclose( fh ) == 0 );
}
END_TEST

START_TEST( fgets )
{
    FILE * fh = NULL;
    char buffer[10];
    const char * fgets_test = "foo\nbar\0baz\nweenie";
    TESTCASE_REQUIRE( ( fh = fopen( testfile, "wb+" ) ) != NULL );
    TESTCASE( fwrite( fgets_test, 1, 18, fh ) == 18 );
    rewind( fh );
    TESTCASE( fgets( buffer, 10, fh ) == buffer );
    TESTCASE( strcmp( buffer, "foo\n" ) == 0 );
    TESTCASE( fgets( buffer, 10, fh ) == buffer );
    TESTCASE( memcmp( buffer, "bar\0baz\n", 8 ) == 0 );
    TESTCASE( fgets( buffer, 10, fh ) == buffer );
    TESTCASE( strcmp( buffer, "weenie" ) == 0 );
    TESTCASE( feof( fh ) );
    TESTCASE( fseek( fh, -1, SEEK_END ) == 0 );
    TESTCASE( fgets( buffer, 1, fh ) == buffer );
    TESTCASE( strcmp( buffer, "" ) == 0 );
    TESTCASE( fgets( buffer, 0, fh ) == NULL );
    TESTCASE( ! feof( fh ) );
    TESTCASE( fgets( buffer, 1, fh ) == buffer );
    TESTCASE( strcmp( buffer, "" ) == 0 );
    TESTCASE( ! feof( fh ) );
    TESTCASE( fgets( buffer, 2, fh ) == buffer );
    TESTCASE( strcmp( buffer, "e" ) == 0 );
    TESTCASE( fseek( fh, 0, SEEK_END ) == 0 );
    TESTCASE( fgets( buffer, 2, fh ) == NULL );
    TESTCASE( feof( fh ) );
    TESTCASE( fclose( fh ) == 0 );
    TESTCASE( remove( testfile ) == 0 );
}
END_TEST

START_TEST( fopen )
{
    /* Some of the tests are not executed for regression tests, as the libc on
       my system is at once less forgiving (segfaults on mode NULL) and more
       forgiving (accepts undefined modes).
    */
    FILE * fh = NULL;
    remove( testfile );
    TESTCASE( fopen( NULL, NULL ) == NULL );
    TESTCASE( fopen( NULL, "w" ) == NULL );
    TESTCASE( fopen( "", NULL ) == NULL );
    TESTCASE( fopen( "", "w" ) == NULL );
    TESTCASE( fopen( "foo", "" ) == NULL );
    TESTCASE( fopen( testfile, "wq" ) == NULL ); /* Undefined mode */
    TESTCASE( fopen( testfile, "wr" ) == NULL ); /* Undefined mode */
    TESTCASE( ( fh = fopen( testfile, "w" ) ) != NULL );
    TESTCASE( fclose( fh ) == 0 );
    TESTCASE( remove( testfile ) == 0 );
}
END_TEST

START_TEST( fputs )
{
    const char * const message = "SUCCESS testing fputs()";
    FILE * fh = tmpfile();
    size_t i;
    TESTCASE_REQUIRE( fh != NULL );
    TESTCASE( fputs( message, fh ) >= 0 );
    rewind( fh );
    for ( i = 0; i < 23; ++i )
    {
        TESTCASE( fgetc( fh ) == message[i] );
    }
    TESTCASE( fclose( fh ) == 0 );
}
END_TEST

START_TEST( fread )
{
    FILE * fh = NULL;
    const char * message = "Testing fwrite()...\n";
    char buffer[21];
    buffer[20] = 'x';
    TESTCASE_REQUIRE( ( fh = tmpfile() ) != NULL );
    /* fwrite() / readback */
    TESTCASE( fwrite( message, 1, 20, fh ) == 20 );
    rewind( fh );
    TESTCASE( fread( buffer, 1, 20, fh ) == 20 );
    TESTCASE( memcmp( buffer, message, 20 ) == 0 );
    TESTCASE( buffer[20] == 'x' );
    /* same, different nmemb / size settings */
    rewind( fh );
    TESTCASE( memset( buffer, '\0', 20 ) == buffer );
    TESTCASE( fwrite( message, 5, 4, fh ) == 4 );
    rewind( fh );
    TESTCASE( fread( buffer, 5, 4, fh ) == 4 );
    TESTCASE( memcmp( buffer, message, 20 ) == 0 );
    TESTCASE( buffer[20] == 'x' );
    /* same... */
    rewind( fh );
    TESTCASE( memset( buffer, '\0', 20 ) == buffer );
    TESTCASE( fwrite( message, 20, 1, fh ) == 1 );
    rewind( fh );
    TESTCASE( fread( buffer, 20, 1, fh ) == 1 );
    TESTCASE( memcmp( buffer, message, 20 ) == 0 );
    TESTCASE( buffer[20] == 'x' );
    /* Done. */
    TESTCASE( fclose( fh ) == 0 );
}
END_TEST

START_TEST( freopen )
{
    FILE * fin = NULL;
    FILE * fout = NULL;
    TESTCASE_REQUIRE( ( fin = fopen( testfile1, "wb+" ) ) != NULL );
    TESTCASE( fputc( 'x', fin ) == 'x' );
    TESTCASE( fclose( fin ) == 0 );
    TESTCASE( ( fin = freopen( testfile1, "rb", stdin ) ) != NULL );
    TESTCASE( getchar() == 'x' );

    TESTCASE( ( fout = freopen( testfile2, "wb+", stdout ) ) != NULL );
    TESTCASE( putchar( 'x' ) == 'x' );
    rewind( fout );
    TESTCASE( fgetc( fout ) == 'x' );

    TESTCASE( fclose( fin ) == 0 );
    TESTCASE( fclose( fout ) == 0 );
    TESTCASE( remove( testfile1 ) == 0 );
    TESTCASE( remove( testfile2 ) == 0 );
}
END_TEST

START_TEST( fseek )
{
    FILE * fh = NULL;
    TESTCASE( ( fh = tmpfile() ) != NULL );
    TESTCASE( fwrite( teststring, 1, strlen( teststring ), fh ) == strlen( teststring ) );
    /* General functionality */
    TESTCASE( fseek( fh, -1, SEEK_END ) == 0  );
    TESTCASE( (size_t)ftell( fh ) == strlen( teststring ) - 1 );
    TESTCASE( fseek( fh, 0, SEEK_END ) == 0 );
    TESTCASE( (size_t)ftell( fh ) == strlen( teststring ) );
    TESTCASE( fseek( fh, 0, SEEK_SET ) == 0 );
    TESTCASE( ftell( fh ) == 0 );
    TESTCASE( fseek( fh, 5, SEEK_CUR ) == 0 );
    TESTCASE( ftell( fh ) == 5 );
    TESTCASE( fseek( fh, -3, SEEK_CUR ) == 0 );
    TESTCASE( ftell( fh ) == 2 );
    /* Checking behaviour around EOF */
    TESTCASE( fseek( fh, 0, SEEK_END ) == 0 );
    TESTCASE( ! feof( fh ) );
    TESTCASE( fgetc( fh ) == EOF );
    TESTCASE( feof( fh ) );
    TESTCASE( fseek( fh, 0, SEEK_END ) == 0 );
    TESTCASE( ! feof( fh ) );
    /* Checking undo of ungetc() */
    TESTCASE( fseek( fh, 0, SEEK_SET ) == 0 );
    TESTCASE( fgetc( fh ) == teststring[0] );
    TESTCASE( fgetc( fh ) == teststring[1] );
    TESTCASE( fgetc( fh ) == teststring[2] );
    TESTCASE( ftell( fh ) == 3 );
    TESTCASE( ungetc( teststring[2], fh ) == teststring[2] );
    TESTCASE( ftell( fh ) == 2 );
    TESTCASE( fgetc( fh ) == teststring[2] );
    TESTCASE( ftell( fh ) == 3 );
    TESTCASE( ungetc( 'x', fh ) == 'x' );
    TESTCASE( ftell( fh ) == 2 );
    TESTCASE( fgetc( fh ) == 'x' );
    TESTCASE( ungetc( 'x', fh ) == 'x' );
    TESTCASE( ftell( fh ) == 2 );
    TESTCASE( fseek( fh, 2, SEEK_SET ) == 0 );
    TESTCASE( fgetc( fh ) == teststring[2] );
    /* Checking error handling */
    TESTCASE( fseek( fh, -5, SEEK_SET ) == -1 );
    TESTCASE( fseek( fh, 0, SEEK_END ) == 0 );
    TESTCASE( fclose( fh ) == 0 );
}
END_TEST

START_TEST( ftell )
{
    /* Testing all the basic I/O functions individually would result in lots
       of duplicated code, so I took the liberty of lumping it all together
       here.
    */
    /* The following functions delegate their tests to here:
       fgetc fflush rewind fputc ungetc fseek
       flushbuffer seek fillbuffer prepread prepwrite
    */
    char * buffer = (char*)malloc( 4 );
    FILE * fh = NULL;
    TESTCASE_REQUIRE( ( fh = tmpfile() ) != NULL );
    TESTCASE( setvbuf( fh, buffer, _IOLBF, 4 ) == 0 );
    /* Testing ungetc() at offset 0 */
    rewind( fh );
    TESTCASE( ungetc( 'x', fh ) == 'x' );
    TESTCASE( ftell( fh ) == -1l );
    rewind( fh );
    TESTCASE( ftell( fh ) == 0l );
    /* Commence "normal" tests */
    TESTCASE( fputc( '1', fh ) == '1' );
    TESTCASE( fputc( '2', fh ) == '2' );
    TESTCASE( fputc( '3', fh ) == '3' );
    /* Positions incrementing as expected? */
    TESTCASE( ftell( fh ) == 3l );
    TESTCASE( fh->pos.offset == 0l );
    TESTCASE( fh->bufidx == 3l );
    /* Buffer properly flushed when full? */
    TESTCASE( fputc( '4', fh ) == '4' );
    TESTCASE( fh->pos.offset == 4l );
    TESTCASE( fh->bufidx == 0 );
    /* fflush() resetting positions as expected? */
    TESTCASE( fputc( '5', fh ) == '5' );
    TESTCASE( fflush( fh ) == 0 );
    TESTCASE( ftell( fh ) == 5l );
    TESTCASE( fh->pos.offset == 5l );
    TESTCASE( fh->bufidx == 0l );
    /* rewind() resetting positions as expected? */
    rewind( fh );
    TESTCASE( ftell( fh ) == 0l );
    TESTCASE( fh->pos.offset == 0 );
    TESTCASE( fh->bufidx == 0 );
    /* Reading back first character after rewind for basic read check */
    TESTCASE( fgetc( fh ) == '1' );
    /* TODO: t.b.c. */
    TESTCASE( fclose( fh ) == 0 );
}
END_TEST

START_TEST( perror )
{
    FILE * fh = NULL;
    unsigned long long max = ULLONG_MAX;
    char buffer[100];
    sprintf( buffer, "%llu", max );
    TESTCASE_REQUIRE( ( fh = freopen( testfile, "wb+", j6libc_stderr ) ) != NULL );
    TESTCASE( strtol( buffer, NULL, 10 ) == LONG_MAX );
    perror( "Test" );
    rewind( fh );
    TESTCASE( fread( buffer, 1, 7, fh ) == 7 );
    TESTCASE( memcmp( buffer, "Test: ", 6 ) == 0 );
    TESTCASE( fclose( fh ) == 0 );
    TESTCASE( remove( testfile ) == 0 );
}
END_TEST

START_TEST( puts )
{
    FILE * fh = NULL;
    const char * message = "SUCCESS testing puts()";
    char buffer[23];
    buffer[22] = 'x';
    TESTCASE_REQUIRE( ( fh = freopen( testfile, "wb+", stdout ) ) != NULL );
    TESTCASE( puts( message ) >= 0 );
    rewind( fh );
    TESTCASE( fread( buffer, 1, 22, fh ) == 22 );
    TESTCASE( memcmp( buffer, message, 22 ) == 0 );
    TESTCASE( buffer[22] == 'x' );
    TESTCASE( fclose( fh ) == 0 );
    TESTCASE( remove( testfile ) == 0 );
}
END_TEST

START_TEST( rename )
{
    FILE * file = NULL;
    remove( testfile1 );
    remove( testfile2 );
    /* make sure that neither file exists */
    TESTCASE( fopen( testfile1, "r" ) == NULL );
    TESTCASE( fopen( testfile2, "r" ) == NULL );
    /* rename file 1 to file 2 - expected to fail */
    TESTCASE( rename( testfile1, testfile2 ) == -1 );
    /* create file 1 */
    TESTCASE_REQUIRE( ( file = fopen( testfile1, "w" ) ) != NULL );
    TESTCASE( fputs( "x", file ) != EOF );
    TESTCASE( fclose( file ) == 0 );
    /* check that file 1 exists */
    TESTCASE( ( file = fopen( testfile1, "r" ) ) != NULL );
    TESTCASE( fclose( file ) == 0 );
    /* rename file 1 to file 2 */
    TESTCASE( rename( testfile1, testfile2 ) == 0 );
    /* check that file 2 exists, file 1 does not */
    TESTCASE( fopen( testfile1, "r" ) == NULL );
    TESTCASE( ( file = fopen( testfile2, "r" ) ) != NULL );
    TESTCASE( fclose( file ) == 0 );
    /* create another file 1 */
    TESTCASE( ( file = fopen( testfile1, "w" ) ) != NULL );
    TESTCASE( fputs( "x", file ) != EOF );
    TESTCASE( fclose( file ) == 0 );
    /* check that file 1 exists */
    TESTCASE( ( file = fopen( testfile1, "r" ) ) != NULL );
    TESTCASE( fclose( file ) == 0 );
    /* rename file 1 to file 2 - expected to fail, see comment in
       _PDCLIB_rename() itself.
    */
    /* NOREG as glibc overwrites existing destination file. */
    TESTCASE( rename( testfile1, testfile2 ) == -1 );
    /* remove both files */
    TESTCASE( remove( testfile1 ) == 0 );
    TESTCASE( remove( testfile2 ) == 0 );
    /* check that they're gone */
    TESTCASE( fopen( testfile1, "r" ) == NULL );
    TESTCASE( fopen( testfile2, "r" ) == NULL );
}
END_TEST

START_TEST( setbuf )
{
    /* TODO: Extend testing once setvbuf() is finished. */
    char buffer[ BUFSIZ + 1 ];
    FILE * fh = NULL;
    /* full buffered */
    TESTCASE_REQUIRE( ( fh = tmpfile() ) != NULL );
    setbuf( fh, buffer );
    TESTCASE( fh->buffer == buffer );
    TESTCASE( fh->bufsize == BUFSIZ );
    TESTCASE( ( fh->status & ( _IOFBF | _IONBF | _IOLBF ) ) == _IOFBF );
    TESTCASE( fclose( fh ) == 0 );
    /* not buffered */
    TESTCASE( ( fh = tmpfile() ) != NULL );
    setbuf( fh, NULL );
    TESTCASE( ( fh->status & ( _IOFBF | _IONBF | _IOLBF ) ) == _IONBF );
    TESTCASE( fclose( fh ) == 0 );
}
END_TEST


START_TEST( setvbuf )
{
#define BUFFERSIZE 500
    char buffer[ BUFFERSIZE ];
    FILE * fh = NULL;
    /* full buffered, user-supplied buffer */
    TESTCASE_REQUIRE( ( fh = tmpfile() ) != NULL );
    TESTCASE( setvbuf( fh, buffer, _IOFBF, BUFFERSIZE ) == 0 );
    TESTCASE( fh->buffer == buffer );
    TESTCASE( fh->bufsize == BUFFERSIZE );
    TESTCASE( ( fh->status & ( _IOFBF | _IONBF | _IOLBF ) ) == _IOFBF );
    TESTCASE( fclose( fh ) == 0 );
    /* line buffered, lib-supplied buffer */
    TESTCASE( ( fh = tmpfile() ) != NULL );
    TESTCASE( setvbuf( fh, NULL, _IOLBF, BUFFERSIZE ) == 0 );
    TESTCASE( fh->buffer != NULL );
    TESTCASE( fh->bufsize == BUFFERSIZE );
    TESTCASE( ( fh->status & ( _IOFBF | _IONBF | _IOLBF ) ) == _IOLBF );
    TESTCASE( fclose( fh ) == 0 );
    /* not buffered, user-supplied buffer */
    TESTCASE( ( fh = tmpfile() ) != NULL );
    TESTCASE( setvbuf( fh, buffer, _IONBF, BUFFERSIZE ) == 0 );
    TESTCASE( ( fh->status & ( _IOFBF | _IONBF | _IOLBF ) ) == _IONBF );
    TESTCASE( fclose( fh ) == 0 );
#undef BUFFERSIZE
}
END_TEST

START_TEST( tmpnam )
{
    TESTCASE( strlen( tmpnam( NULL ) ) < L_tmpnam );
}
END_TEST


FILE * test_vfprintf_stream = NULL;

static int test_vfprintf(
        const char *implfile,
        const char *file,
        int line,
        size_t expected_rc,
        const char *expected_out,
        const char *format, 
        ... )
{
    size_t i;
    va_list arg;
    va_start( arg, format );
    i = vfprintf( test_vfprintf_stream, format, arg );
    va_end( arg );

    char result_buffer[100];
    rewind(test_vfprintf_stream);
    if ( fread( result_buffer, 1, i, test_vfprintf_stream ) != i )
    {
        fprintf( stderr,
            "FAILED: %s (%s), line %d\n"
            "        GET RESULT FAILURE",
            file, implfile, line);
        return 1;
    }

    if (i != expected_rc ||
        strcmp(result_buffer, expected_out))
    {
        fprintf( stderr,
            "FAILED: %s (%s), line %d\n"
            "        format string \"%s\"\n"
            "        expected %2lu, \"%s\"\n"
            "        actual   %2lu, \"%s\"\n",
             file, implfile, line, format, expected_rc,
             expected_out, i, result_buffer );
        return 1;
    }

    return 0;
}

START_TEST( vfprintf )
{
    TESTCASE_REQUIRE( ( test_vfprintf_stream = tmpfile() ) != NULL );
#define IMPLFILE "stdio/vfprintf.c"
#define DO_TESTPRINTF test_vfprintf
#include "printf_testcases.h"
#undef DO_TESTPRINTF
#undef IMPLFILE
    TESTCASE( fclose( test_vfprintf_stream ) == 0 );
}
END_TEST

FILE * test_vfscanf_stream = NULL;

static int test_vfscanf(
        const char *implfile,
        const char *file,
        int line,
        size_t expected_rc,
        const char *input_string,
        const char *format, 
        ... )
{
    rewind( test_vfscanf_stream );
    fwrite( input_string, 1, strlen(input_string)+1, test_vfscanf_stream );
    rewind( test_vfscanf_stream );

    va_list ap;
    size_t result;
    va_start( ap, format );
    result = vfscanf( test_vfscanf_stream, format, ap );
    va_end( ap );

    if (result != expected_rc )
    {
        fprintf( stderr,
            "FAILED: %s (%s), line %d\n"
            "        format string \"%s\"\n"
            "        expected %2lu\n"
            "        actual   %2lu\n",
             file, implfile, line, format,
             expected_rc, result);
        return 1;
    }

    return 0;
}

START_TEST( vfscanf )
{
    TESTCASE_REQUIRE( ( test_vfscanf_stream = tmpfile() ) != NULL );
#define IMPLFILE "stdio/vfscanf.c"
#define DO_TESTSCANF test_vfscanf
#include "scanf_testcases.h"
#undef DO_TESTSCANF
#undef IMPLFILE
    TESTCASE( fclose( test_vfprintf_stream ) == 0 );
}
END_TEST

static int test_vsnprintf(
        const char *implfile,
        const char *file,
        int line,
        size_t expected_rc,
        const char *expected_out,
        const char *format, 
        ... )
{
    char buffer[100];
    size_t i;
    va_list arg;
    va_start( arg, format );
    i = vsnprintf( buffer, 100, format, arg );
    va_end( arg );

    if (i != expected_rc ||
        strcmp(buffer, expected_out))
    {
        fprintf( stderr,
            "FAILED: %s (%s), line %d\n"
            "        format string \"%s\"\n"
            "        expected %2lu, \"%s\"\n"
            "        actual   %2lu, \"%s\"\n",
             file, implfile, line, format, expected_rc,
             expected_out, i, buffer );
        return 1;
    }

    return 0;
}

START_TEST( vsnprintf )
{
#define IMPLFILE "stdio/vsnprintf.c"
#define DO_TESTPRINTF test_vsnprintf
#include "printf_testcases.h"
#undef DO_TESTPRINTF
#undef IMPLFILE
}
END_TEST

static int test_vsscanf(
        const char *implfile,
        const char *file,
        int line,
        size_t expected_rc,
        const char *input_string,
        const char *format, 
        ... )
{
    va_list ap;
    size_t result;
    va_start( ap, format );
    result = vsscanf( input_string, format, ap );
    va_end( ap );

    if (result != expected_rc )
    {
        fprintf( stderr,
            "FAILED: %s (%s), line %d\n"
            "        format string \"%s\"\n"
            "        expected %2lu\n"
            "        actual   %2lu\n",
             file, implfile, line, format,
             expected_rc, result);
        return 1;
    }

    return 0;
}

START_TEST( vsscanf )
{
#define IMPLFILE "stdio/vsscanf.c"
#define DO_TESTSCANF test_vsscanf
#include "scanf_testcases.h"
#undef DO_TESTSCANF
#undef IMPLFILE
}
END_TEST


START_SUITE( stdio )
{
    /* TODO: File IO not yet implemented
    RUN_TEST( clearerr );
    RUN_TEST( fclose );
    RUN_TEST( fgetpos );
    RUN_TEST( fgets );
    RUN_TEST( fopen );
    RUN_TEST( fputs );
    RUN_TEST( fread );
    RUN_TEST( freopen );
    RUN_TEST( fseek );
    RUN_TEST( ftell );
    RUN_TEST( perror );
    RUN_TEST( puts );
    RUN_TEST( rename );
    RUN_TEST( setbuf );
    RUN_TEST( setvbuf );
    RUN_TEST( vfprintf );
    RUN_TEST( vfscanf );
    */

    RUN_TEST( tmpnam );
    RUN_TEST( vsnprintf );
    RUN_TEST( vsscanf );
}
END_SUITE
