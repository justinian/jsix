/* _PDCLIB_load_lc_ctype( const char *, const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "j6libc/int.h"

struct _PDCLIB_lc_ctype_t * _PDCLIB_load_lc_ctype( const char * path, const char * locale )
{
    struct _PDCLIB_lc_ctype_t * rc = NULL;
    const char * extension = "_ctype.dat";
    char * file = malloc( strlen( path ) + strlen( locale ) + strlen( extension ) + 1 );

    if ( file )
    {
        FILE * fh;

        strcpy( file, path );
        strcat( file, locale );
        strcat( file, extension );

        if ( ( fh = fopen( file, "rb" ) ) != NULL )
        {
            if ( ( rc = malloc( sizeof( struct _PDCLIB_lc_ctype_t ) ) ) != NULL )
            {
                struct _PDCLIB_lc_ctype_entry_t * entry;

                if ( ( entry = malloc( sizeof( struct _PDCLIB_lc_ctype_entry_t ) * _PDCLIB_CHARSET_SIZE + 1 ) ) != NULL )
                {
                    rc->entry = entry + 1;
                    rc->entry[ -1 ].flags = rc->entry[ -1 ].upper = rc->entry[ -1 ].lower =  0;

                    if ( fscanf( fh, "%x %x %x %x %x %x", &rc->digits_low, &_PDCLIB_lc_ctype.digits_high, &_PDCLIB_lc_ctype.Xdigits_low, &_PDCLIB_lc_ctype.Xdigits_high, &_PDCLIB_lc_ctype.xdigits_low, &_PDCLIB_lc_ctype.xdigits_high ) == 6 )
                    {
                        size_t i;

                        for ( i = 0; i < _PDCLIB_CHARSET_SIZE; ++i )
                        {
                            if ( fscanf( fh, "%hx %hhx %hhx", &rc->entry[ i ].flags, &rc->entry[ i ].upper, &rc->entry[ i ].lower ) != 3 )
                            {
                                fclose( fh );
                                free( file );
                                free( rc->entry - 1 );
                                free( rc );
                                return NULL;
                            }
                        }
                    }

                    rc->alloced = 1;
                }
                else
                {
                    free( rc );
                }
            }

            fclose( fh );
        }

        free( file );
    }

    return rc;
}
