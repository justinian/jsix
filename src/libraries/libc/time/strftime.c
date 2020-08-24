/* strftime( char * restrict, size_t, const char * restrict, const struct tm * restrict )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <time.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <assert.h>

/* TODO: Alternative representations / numerals not supported. Multibyte support missing. */

/* This implementation's code is highly repetitive, but I did not really
   care for putting it into a number of macros / helper functions.
*/

enum wstart_t
{
    E_SUNDAY = 0,
    E_MONDAY = 1,
    E_ISO_WEEK,
    E_ISO_YEAR
};

#include <stdio.h>

static int week_calc( const struct tm * timeptr, int wtype )
{
    int wday;
    int bias;
    int week;

    if ( wtype <= E_MONDAY )
    {
        /* Simple -- first week starting with E_SUNDAY / E_MONDAY,
           days before that are week 0.
        */
        div_t weeks = div( timeptr->tm_yday, 7 );
        wday = ( timeptr->tm_wday + 7 - wtype ) % 7;
        if ( weeks.rem >= wday )
        {
            ++weeks.quot;
        }
        return weeks.quot;
    }

    /* calculating ISO week; relies on Sunday == 7 */
    wday = timeptr->tm_wday;
    if ( wday == 0 )
    {
        wday = 7;
    }
    /* https://en.wikipedia.org/wiki/ISO_week_date */
    week = ( timeptr->tm_yday - wday + 11 ) / 7;
    if ( week == 53 )
    {
        /* date *may* belong to the *next* year, if:
           * it is 31.12. and Monday - Wednesday
           * it is 30.12. and Monday - Tuesday
           * it is 29.12. and Monday
           We can safely assume December...
        */
        if ( ( timeptr->tm_yday - wday - _PDCLIB_is_leap( timeptr->tm_year ) ) > 360 )
        {
            week = 1;
        }
    }
    else if ( week == 0 )
    {
        /* date *does* belong to *previous* year,
           i.e. has week 52 *unless*...
           * current year started on a Friday, or
           * previous year is leap and this year
             started on a Saturday.
        */
        int firstday = timeptr->tm_wday - ( timeptr->tm_yday % 7 );
        if ( firstday < 0 )
        {
            firstday += 7;
        }
        if ( ( firstday == 5 ) || ( _PDCLIB_is_leap( timeptr->tm_year - 1 ) && firstday == 6 ) )
        {
            week = 53;
        }
        else
        {
            week = 52;
        }
    }
    if ( wtype == E_ISO_WEEK )
    {
        return week;
    }

    /* E_ISO_YEAR -- determine the "week-based year" */
    bias = 0;
    if ( week >= 52 && timeptr->tm_mon == 0 )
    {
        --bias;
    }
    else if ( week == 1 && timeptr->tm_mon == 11 )
    {
        ++bias;
    }
    return timeptr->tm_year + 1900 + bias;
}

/* Assuming presence of s, rc, maxsize.
   Checks index for valid range, target buffer for sufficient remaining
   capacity, and copies the locale-specific string (or "?" if index out
   of range). Returns with zero if buffer capacity insufficient.
*/
#define SPRINTSTR( array, index, max ) \
    { \
        int ind = (index); \
        const char * str = "?"; \
        size_t len; \
        if ( ind >= 0 && ind <= max ) \
        { \
            str = array[ ind ]; \
        } \
        len = strlen( str ); \
        if ( rc < ( maxsize - len ) ) \
        { \
            strcpy( s + rc, str ); \
            rc += len; \
        } \
        else \
        { \
            return 0; \
        } \
    }

#define SPRINTREC( format ) \
    { \
        size_t count = strftime( s + rc, maxsize - rc, format, timeptr ); \
        if ( count == 0 ) \
        { \
            return 0; \
        } \
        else \
        { \
            rc += count; \
        } \
    }

size_t strftime( char * restrict s, size_t maxsize, const char * restrict format, const struct tm * restrict timeptr )
{
    size_t rc = 0;

    while ( rc < maxsize )
    {
        if ( *format != '%' )
        {
            if ( ( s[rc] = *format++ ) == '\0' )
            {
                return rc;
            }
            else
            {
                ++rc;
            }
        }
        else
        {
            /* char flag = 0; */
            switch ( *++format )
            {
                case 'E':
                case 'O':
                    /* flag = *format++; */
                    break;
                default:
                    /* EMPTY */
                    break;
            }
            switch( *format++ )
            {
                case 'a':
                    {
                        /* tm_wday abbreviated */
                        SPRINTSTR( _PDCLIB_lc_time.day_name_abbr, timeptr->tm_wday, 6 );
                        break;
                    }
                case 'A':
                    {
                        /* tm_wday full */
                        SPRINTSTR( _PDCLIB_lc_time.day_name_full, timeptr->tm_wday, 6 );
                        break;
                    }
                case 'b':
                case 'h':
                    {
                        /* tm_mon abbreviated */
                        SPRINTSTR( _PDCLIB_lc_time.month_name_abbr, timeptr->tm_mon, 11 );
                        break;
                    }
                case 'B':
                    {
                        /* tm_mon full */
                        SPRINTSTR( _PDCLIB_lc_time.month_name_full, timeptr->tm_mon, 11 );
                        break;
                    }
                case 'c':
                    {
                        /* locale's date / time representation, %a %b %e %T %Y for C locale */
                        /* 'E' for locale's alternative representation */
                        SPRINTREC( _PDCLIB_lc_time.date_time_format );
                        break;
                    }
                case 'C':
                    {
                        /* tm_year divided by 100, truncated to decimal (00-99) */
                        /* 'E' for base year (period) in locale's alternative representation */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t period = div( ( ( timeptr->tm_year + 1900 ) / 100 ), 10 );
                            s[rc++] = '0' + period.quot;
                            s[rc++] = '0' + period.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'd':
                    {
                        /* tm_mday as decimal (01-31) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t day = div( timeptr->tm_mday, 10 );
                            s[rc++] = '0' + day.quot;
                            s[rc++] = '0' + day.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'D':
                    {
                        /* %m/%d/%y */
                        SPRINTREC( "%m/%d/%y" );
                        break;
                    }
                case 'e':
                    {
                        /* tm_mday as decimal ( 1-31) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t day = div( timeptr->tm_mday, 10 );
                            s[rc++] = ( day.quot > 0 ) ? '0' + day.quot : ' ';
                            s[rc++] = '0' + day.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'F':
                    {
                        /* %Y-%m-%d */
                        SPRINTREC( "%Y-%m-%d" );
                        break;
                    }
                case 'g':
                    {
                        /* last 2 digits of the week-based year as decimal (00-99) */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t year = div( week_calc( timeptr, E_ISO_YEAR ) % 100, 10 );
                            s[rc++] = '0' + year.quot;
                            s[rc++] = '0' + year.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'G':
                    {
                        /* week-based year as decimal (e.g. 1997) */
                        if ( rc < ( maxsize - 4 ) )
                        {
                            int year = week_calc( timeptr, E_ISO_YEAR );
                            int i;
                            for ( i = 3; i >= 0; --i )
                            {
                                div_t digit = div( year, 10 );
                                s[ rc + i ] = '0' + digit.rem;
                                year = digit.quot;
                            }

                            rc += 4;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'H':
                    {
                        /* tm_hour as 24h decimal (00-23) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t hour = div( timeptr->tm_hour, 10 );
                            s[rc++] = '0' + hour.quot;
                            s[rc++] = '0' + hour.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'I':
                    {
                        /* tm_hour as 12h decimal (01-12) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t hour = div( ( timeptr->tm_hour + 11 ) % 12 + 1, 10 );
                            s[rc++] = '0' + hour.quot;
                            s[rc++] = '0' + hour.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'j':
                    {
                        /* tm_yday as decimal (001-366) */
                        if ( rc < ( maxsize - 3 ) )
                        {
                            div_t yday = div( timeptr->tm_yday + 1, 100 );
                            s[rc++] = '0' + yday.quot;
                            s[rc++] = '0' + yday.rem / 10;
                            s[rc++] = '0' + yday.rem % 10;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'm':
                    {
                        /* tm_mon as decimal (01-12) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t mon = div( timeptr->tm_mon + 1, 10 );
                            s[rc++] = '0' + mon.quot;
                            s[rc++] = '0' + mon.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'M':
                    {
                        /* tm_min as decimal (00-59) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t min = div( timeptr->tm_min, 10 );
                            s[rc++] = '0' + min.quot;
                            s[rc++] = '0' + min.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'n':
                    {
                        /* newline */
                        s[rc++] = '\n';
                        break;
                    }
                case 'p':
                    {
                        /* tm_hour locale's AM/PM designations */
                        SPRINTSTR( _PDCLIB_lc_time.am_pm, timeptr->tm_hour > 11, 1 );
                        break;
                    }
                case 'r':
                    {
                        /* tm_hour / tm_min / tm_sec as locale's 12-hour clock time, %I:%M:%S %p for C locale */
                        SPRINTREC( _PDCLIB_lc_time.time_format_12h );
                        break;
                    }
                case 'R':
                    {
                        /* %H:%M */
                        SPRINTREC( "%H:%M" );
                        break;
                    }
                case 'S':
                    {
                        /* tm_sec as decimal (00-60) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t sec = div( timeptr->tm_sec, 10 );
                            s[rc++] = '0' + sec.quot;
                            s[rc++] = '0' + sec.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 't':
                    {
                        /* tabulator */
                        s[rc++] = '\t';
                        break;
                    }
                case 'T':
                    {
                        /* %H:%M:%S */
                        SPRINTREC( "%H:%M:%S" );
                        break;
                    }
                case 'u':
                    {
                        /* tm_wday as decimal (1-7) with Monday == 1 */
                        /* 'O' for locale's alternative numeric symbols */
                        s[rc++] = ( timeptr->tm_wday == 0 ) ? '7' : '0' + timeptr->tm_wday;
                        break;
                    }
                case 'U':
                    {
                        /* week number of the year (first Sunday as the first day of week 1) as decimal (00-53) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t week = div( week_calc( timeptr, E_SUNDAY ), 10 );
                            s[rc++] = '0' + week.quot;
                            s[rc++] = '0' + week.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'V':
                    {
                        /* ISO week number as decimal (01-53) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t week = div( week_calc( timeptr, E_ISO_WEEK ), 10 );
                            s[rc++] = '0' + week.quot;
                            s[rc++] = '0' + week.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'w':
                    {
                        /* tm_wday as decimal number (0-6) with Sunday == 0 */
                        /* 'O' for locale's alternative numeric symbols */
                        s[rc++] = '0' + timeptr->tm_wday;
                        break;
                    }
                case 'W':
                    {
                        /* week number of the year (first Monday as the first day of week 1) as decimal (00-53) */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t week = div( week_calc( timeptr, E_MONDAY ), 10 );
                            s[rc++] = '0' + week.quot;
                            s[rc++] = '0' + week.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'x':
                    {
                        /* locale's date representation, %m/%d/%y for C locale */
                        /* 'E' for locale's alternative representation */
                        SPRINTREC( _PDCLIB_lc_time.date_format );
                        break;
                    }
                case 'X':
                    {
                        /* locale's time representation, %T for C locale */
                        /* 'E' for locale's alternative representation */
                        SPRINTREC( _PDCLIB_lc_time.time_format );
                        break;
                    }
                case 'y':
                    {
                        /* last 2 digits of tm_year as decimal (00-99) */
                        /* 'E' for offset from %EC (year only) in locale's alternative representation */
                        /* 'O' for locale's alternative numeric symbols */
                        if ( rc < ( maxsize - 2 ) )
                        {
                            div_t year = div( ( timeptr->tm_year % 100 ), 10 );
                            s[rc++] = '0' + year.quot;
                            s[rc++] = '0' + year.rem;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'Y':
                    {
                        /* tm_year as decimal (e.g. 1997) */
                        /* 'E' for locale's alternative representation */
                        if ( rc < ( maxsize - 4 ) )
                        {
                            int year = timeptr->tm_year + 1900;
                            int i;

                            for ( i = 3; i >= 0; --i )
                            {
                                div_t digit = div( year, 10 );
                                s[ rc + i ] = '0' + digit.rem;
                                year = digit.quot;
                            }

                            rc += 4;
                        }
                        else
                        {
                            return 0;
                        }
                        break;
                    }
                case 'z':
                    {
                        /* tm_isdst / UTC offset in ISO8601 format (e.g. -0430 meaning 4 hours 30 minutes behind Greenwich), or no characters */
                        /* TODO: 'z' */
                        break;
                    }
                case 'Z':
                    {
                        /* tm_isdst / locale's time zone name or abbreviation, or no characters */
                        /* TODO: 'Z' */
                        break;
                    }
                case '%':
                    {
                        /* '%' character */
                        s[rc++] = '%';
                        break;
                    }
            }
        }
    }

    return 0;
}
