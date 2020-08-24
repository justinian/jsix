/* _PDCLIB_load_lc_time( const char *, const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "j6libc/int.h"

struct _PDCLIB_lc_time_t * _PDCLIB_load_lc_time( const char * path, const char * locale )
{
    struct _PDCLIB_lc_time_t * rc = NULL;
    const char * extension = "_time.dat";
    char * file = malloc( strlen( path ) + strlen( locale ) + strlen( extension ) + 1 );

    if ( file )
    {
        FILE * fh;

        strcpy( file, path );
        strcat( file, locale );
        strcat( file, extension );

        if ( ( fh = fopen( file, "rb" ) ) != NULL )
        {
            if ( ( rc = malloc( sizeof( struct _PDCLIB_lc_time_t ) ) ) != NULL )
            {
                char * data = _PDCLIB_load_lines( fh, 44 );

                if ( data != NULL )
                {
                    size_t i;

                    for ( i = 0; i < 12; ++i )
                    {
                        rc->month_name_abbr[ i ] = data;
                        data += strlen( data ) + 1;
                    }

                    for ( i = 0; i < 12; ++i )
                    {
                        rc->month_name_full[ i ] = data;
                        data += strlen( data ) + 1;
                    }

                    for ( i = 0; i < 7; ++i )
                    {
                        rc->day_name_abbr[ i ] = data;
                        data += strlen( data ) + 1;
                    }

                    for ( i = 0; i < 7; ++i )
                    {
                        rc->day_name_full[ i ] = data;
                        data += strlen( data ) + 1;
                    }

                    rc->alloced = 1;
                }
                else
                {
                    free( rc );
                    rc = NULL;
                }
            }

            fclose( fh );
        }

        free( file );
    }

    return rc;
}
