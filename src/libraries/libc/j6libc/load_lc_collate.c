/* _PDCLIB_load_lc_collate( const char *, const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "j6libc/int.h"

struct _PDCLIB_lc_collate_t * _PDCLIB_load_lc_collate( const char * path, const char * locale )
{
    struct _PDCLIB_lc_collate_t * rc = NULL;
    const char * extension = "_collate.dat";
    char * file = malloc( strlen( path ) + strlen( locale ) + strlen( extension ) + 1 );

    if ( file )
    {
        FILE * fh;

        strcpy( file, path );
        strcat( file, locale );
        strcat( file, extension );

        if ( ( fh = fopen( file, "rb" ) ) != NULL )
        {
            if ( ( rc = malloc( sizeof( struct _PDCLIB_lc_collate_t ) ) ) != NULL )
            {
                /* TODO: Collation data */

                rc->alloced = 1;
            }

            fclose( fh );
        }

        free( file );
    }

    return rc;
}
