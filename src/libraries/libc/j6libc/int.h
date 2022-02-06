#pragma once
/* PDCLib internal integer logic <int.h>

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

/* -------------------------------------------------------------------------- */
/* You should not have to edit anything in this file; if you DO have to, it   */
/* would be considered a bug / missing feature: notify the author(s).         */
/* -------------------------------------------------------------------------- */

#include <stdbool.h>

#include "j6libc/config.h"
#include "j6libc/cpp.h"
#include "j6libc/aux.h"
#include "j6libc/int_widths.h"
#include "j6libc/size_t.h"
#include "j6libc/wchar_t.h"

CPP_CHECK_BEGIN

/* -------------------------------------------------------------------------- */
/* Various <stdio.h> internals                                                */
/* -------------------------------------------------------------------------- */

/* Flags for representing mode (see fopen()). Note these must fit the same
   status field as the _IO?BF flags in <stdio.h> and the internal flags below.
*/
#define _PDCLIB_FREAD     8u
#define _PDCLIB_FWRITE   16u
#define _PDCLIB_FAPPEND  32u
#define _PDCLIB_FRW      64u
#define _PDCLIB_FBIN    128u

/* Internal flags, made to fit the same status field as the flags above. */
/* -------------------------------------------------------------------------- */
/* free() the buffer memory on closing (false for user-supplied buffer) */
#define _PDCLIB_FREEBUFFER   512u
/* stream has encountered error / EOF */
#define _PDCLIB_ERRORFLAG   1024u
#define _PDCLIB_EOFFLAG     2048u
/* stream is wide-oriented */
#define _PDCLIB_WIDESTREAM  4096u
/* stream is byte-oriented */
#define _PDCLIB_BYTESTREAM  8192u
/* file associated with stream should be remove()d on closing (tmpfile()) */
#define _PDCLIB_DELONCLOSE 16384u
/* stream handle should not be free()d on close (stdin, stdout, stderr) */
#define _PDCLIB_STATIC     32768u

/* Position / status structure for getpos() / fsetpos(). */
struct _PDCLIB_fpos_t
{
    uint64_t         offset; /* File position offset */
    int              status; /* Multibyte parsing state (unused, reserved) */
};

/* FILE structure */
struct _PDCLIB_file_t
{
    _PDCLIB_fd_t            handle;   /* OS file handle */
    char *                  buffer;   /* Pointer to buffer memory */
    size_t                  bufsize;  /* Size of buffer */
    size_t                  bufidx;   /* Index of current position in buffer */
    size_t                  bufend;   /* Index of last pre-read character in buffer */
    struct _PDCLIB_fpos_t   pos;      /* Offset and multibyte parsing state */
    size_t                  ungetidx; /* Number of ungetc()'ed characters */
    unsigned char *         ungetbuf; /* ungetc() buffer */
    unsigned int            status;   /* Status flags; see above */
    /* multibyte parsing status to be added later */
    char *                  filename; /* Name the current stream has been opened with */
    struct _PDCLIB_file_t * next;     /* Pointer to next struct (internal) */
};

/* -------------------------------------------------------------------------- */
/* Various <time.h> internals                                                 */
/* -------------------------------------------------------------------------- */

typedef _PDCLIB_time            _PDCLIB_time_t;
typedef _PDCLIB_clock           _PDCLIB_clock_t;

/* -------------------------------------------------------------------------- */
/* Internal data types                                                        */
/* -------------------------------------------------------------------------- */

/* Structure required by both atexit() and exit() for handling atexit functions */
struct _PDCLIB_exitfunc_t
{
    struct _PDCLIB_exitfunc_t * next;
    void (*func)( void );
};

/* Structures required by malloc(), realloc(), and free(). */
struct _PDCLIB_headnode_t
{
    struct _PDCLIB_memnode_t * first;
    struct _PDCLIB_memnode_t * last;
};

struct _PDCLIB_memnode_t
{
    size_t size;
    struct _PDCLIB_memnode_t * next;
};

/* Status structure required by _PDCLIB_print(). */
struct _PDCLIB_status_t
{
    int              base;   /* base to which the value shall be converted   */
    int_fast32_t     flags;  /* flags and length modifiers                */
    size_t           n;      /* print: maximum characters to be written      */
                             /* scan:  number matched conversion specifiers  */
    size_t           i;      /* number of characters read/written            */
    size_t           current;/* chars read/written in the CURRENT conversion */
    char *           s;      /* *sprintf(): target buffer                    */
                             /* *sscanf():  source string                    */
    size_t           width;  /* specified field width                        */
    int              prec;   /* specified field precision                    */
    struct _PDCLIB_file_t * stream; /* *fprintf() / *fscanf() stream         */
    _PDCLIB_va_list  arg;    /* argument stack                               */
};

/* -------------------------------------------------------------------------- */
/* Declaration of helper functions (implemented in functions/_PDCLIB).        */
/* -------------------------------------------------------------------------- */

/* This is the main function called by atoi(), atol() and atoll().            */
intmax_t _PDCLIB_atomax( const char * s );

/* Two helper functions used by strtol(), strtoul() and long long variants.   */
const char * _PDCLIB_strtox_prelim( const char * p, char * sign, int * base );
uintmax_t _PDCLIB_strtox_main( const char ** p, unsigned int base, uintmax_t error, uintmax_t limval, int limdigit, char * sign );

/* Digits arrays used by various integer conversion functions */
extern const char _PDCLIB_digits[];
extern const char _PDCLIB_Xdigits[];

/* The worker for all printf() type of functions. The pointer spec should point
   to the introducing '%' of a conversion specifier. The status structure is to
   be that of the current printf() function, of which the members n, s, stream
   and arg will be preserved; i will be updated; and all others will be trashed
   by the function.
   Returns a pointer to the first character not parsed as conversion specifier.
*/
const char * _PDCLIB_print( const char * spec, struct _PDCLIB_status_t * status );

/* The worker for all scanf() type of functions. The pointer spec should point
   to the introducing '%' of a conversion specifier. The status structure is to
   be that of the current scanf() function, of which the member stream will be
   preserved; n, i, and s will be updated; and all others will be trashed by
   the function.
   Returns a pointer to the first character not parsed as conversion specifier,
   or NULL in case of error.
   FIXME: Should distinguish between matching and input error
*/
const char * _PDCLIB_scan( const char * spec, struct _PDCLIB_status_t * status );

/* Parsing any fopen() style filemode string into a number of flags. */
unsigned int _PDCLIB_filemode( const char * mode );

/* Sanity checking and preparing of read buffer, should be called first thing
   by any stdio read-data function.
   Returns 0 on success, EOF on error.
   On error, EOF / error flags and errno are set appropriately.
*/
int _PDCLIB_prepread( struct _PDCLIB_file_t * stream );

/* Sanity checking, should be called first thing by any stdio write-data
   function.
   Returns 0 on success, EOF on error.
   On error, error flags and errno are set appropriately.
*/
int _PDCLIB_prepwrite( struct _PDCLIB_file_t * stream );

/* Closing all streams on program exit */
void _PDCLIB_closeall( void );

/* Check if a given year is a leap year. Parameter is offset to 1900. */
int _PDCLIB_is_leap( int year_offset );

/* Read a specified number of lines from a file stream; return a pointer to
   allocated memory holding the lines (newlines replaced with zero terminators)
   or NULL in case of error.
*/
char * _PDCLIB_load_lines( struct _PDCLIB_file_t * fh, size_t lines );

/* -------------------------------------------------------------------------- */
/* errno                                                                      */
/* -------------------------------------------------------------------------- */

/* If PDCLib would call its error number "errno" directly, there would be no way
   to catch its value from underlying system calls that also use it (i.e., POSIX
   operating systems). That is why we use an internal name, providing a means to
   access it through <errno.h>.
*/
extern int _PDCLIB_errno;

/* A mechanism for delayed evaluation. (Not sure if this is really necessary, so
   no detailed documentation on the "why".)
*/
int * _PDCLIB_errno_func( void );

/* -------------------------------------------------------------------------- */
/* <locale.h> support                                                         */
/* -------------------------------------------------------------------------- */

#define _PDCLIB_LC_ALL        0
#define _PDCLIB_LC_COLLATE    1
#define _PDCLIB_LC_CTYPE      2
#define _PDCLIB_LC_MONETARY   3
#define _PDCLIB_LC_NUMERIC    4
#define _PDCLIB_LC_TIME       5
#define _PDCLIB_LC_MESSAGES   6
#define _PDCLIB_LC_COUNT      7

#define _PDCLIB_CTYPE_ALPHA   1
#define _PDCLIB_CTYPE_BLANK   2
#define _PDCLIB_CTYPE_CNTRL   4
#define _PDCLIB_CTYPE_GRAPH   8
#define _PDCLIB_CTYPE_PUNCT  16
#define _PDCLIB_CTYPE_SPACE  32
#define _PDCLIB_CTYPE_LOWER  64
#define _PDCLIB_CTYPE_UPPER 128

#define _PDCLIB_CHARSET_SIZE ( 1 << __CHAR_BIT__ )

struct _PDCLIB_lc_lconv_numeric_t
{
    char * decimal_point;
    char * thousands_sep;
    char * grouping;
};

struct _PDCLIB_lc_lconv_monetary_t
{
    char * mon_decimal_point;
    char * mon_thousands_sep;
    char * mon_grouping;
    char * positive_sign;
    char * negative_sign;
    char * currency_symbol;
    char * int_curr_symbol;
    char frac_digits;
    char p_cs_precedes;
    char n_cs_precedes;
    char p_sep_by_space;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    char int_frac_digits;
    char int_p_cs_precedes;
    char int_n_cs_precedes;
    char int_p_sep_by_space;
    char int_n_sep_by_space;
    char int_p_sign_posn;
    char int_n_sign_posn;
};

struct _PDCLIB_lc_numeric_monetary_t
{
    struct lconv * lconv;
    int numeric_alloced;
    int monetary_alloced;
};

extern struct _PDCLIB_lc_numeric_monetary_t _PDCLIB_lc_numeric_monetary;

struct _PDCLIB_lc_collate_t
{
    int alloced;
    /* 1..3 code points */
    /* 1..8, 18 collation elements of 3 16-bit integers */
};

extern struct _PDCLIB_lc_collate_t _PDCLIB_lc_collate;

struct _PDCLIB_lc_ctype_entry_t
{
    uint16_t flags;
    unsigned char upper;
    unsigned char lower;
};

struct _PDCLIB_lc_ctype_t
{
    int alloced;
    int digits_low;
    int digits_high;
    int Xdigits_low;
    int Xdigits_high;
    int xdigits_low;
    int xdigits_high;
    struct _PDCLIB_lc_ctype_entry_t * entry;
};

extern struct _PDCLIB_lc_ctype_t _PDCLIB_lc_ctype;

struct _PDCLIB_lc_messages_t
{
    int alloced;
    char * errno_texts[_PDCLIB_ERRNO_MAX]; /* strerror() / perror()   */
};

extern struct _PDCLIB_lc_messages_t _PDCLIB_lc_messages;

struct _PDCLIB_lc_time_t
{
    int alloced;
    char * month_name_abbr[12]; /* month names, abbreviated                   */
    char * month_name_full[12]; /* month names, full                          */
    char * day_name_abbr[7];    /* weekday names, abbreviated                 */
    char * day_name_full[7];    /* weekday names, full                        */
    char * date_time_format;    /* date / time format for strftime( "%c" )    */
    char * time_format_12h;     /* 12-hour time format for strftime( "%r" )   */
    char * date_format;         /* date format for strftime( "%x" )           */
    char * time_format;         /* time format for strftime( "%X" )           */
    char * am_pm[2];            /* AM / PM designation                        */
};

extern struct _PDCLIB_lc_time_t _PDCLIB_lc_time;

struct _PDCLIB_lc_lconv_numeric_t * _PDCLIB_load_lc_numeric( const char * path, const char * locale );
struct _PDCLIB_lc_lconv_monetary_t * _PDCLIB_load_lc_monetary( const char * path, const char * locale );
struct _PDCLIB_lc_collate_t * _PDCLIB_load_lc_collate( const char * path, const char * locale );
struct _PDCLIB_lc_ctype_t * _PDCLIB_load_lc_ctype( const char * path, const char * locale );
struct _PDCLIB_lc_time_t * _PDCLIB_load_lc_time( const char * path, const char * locale );
struct _PDCLIB_lc_messages_t * _PDCLIB_load_lc_messages( const char * path, const char * locale );

/* -------------------------------------------------------------------------- */
/* Sanity checks                                                              */
/* -------------------------------------------------------------------------- */

_PDCLIB_static_assert( sizeof( short ) == _PDCLIB_SHRT_BYTES, "Compiler disagrees on _PDCLIB_SHRT_BYTES." );
_PDCLIB_static_assert( sizeof( int ) == _PDCLIB_INT_BYTES, "Compiler disagrees on _PDCLIB_INT_BYTES." );
_PDCLIB_static_assert( sizeof( long ) == _PDCLIB_LONG_BYTES, "Compiler disagrees on _PDCLIB_LONG_BYTES." );
_PDCLIB_static_assert( sizeof( long long ) == _PDCLIB_LLONG_BYTES, "Compiler disagrees on _PDCLIB_LLONG_BYTES." );

_PDCLIB_static_assert( ( (char)-1 < 0 ) == _PDCLIB_CHAR_SIGNED, "Compiler disagrees on _PDCLIB_CHAR_SIGNED." );

_PDCLIB_static_assert( sizeof( sizeof( int ) ) == sizeof( size_t ), "Compiler disagrees on size_t." );
_PDCLIB_static_assert( sizeof( wchar_t ) == sizeof( L'x' ), "Compiler disagrees on _PDCLIB_wchar." );
_PDCLIB_static_assert( sizeof( void * ) == sizeof( intptr_t ), "Compiler disagrees on intptr." );
_PDCLIB_static_assert( sizeof( &_PDCLIB_digits[1] - &_PDCLIB_digits[0] ) == sizeof( ptrdiff_t ), "Compiler disagrees on ptrdiff_t." );

CPP_CHECK_END
