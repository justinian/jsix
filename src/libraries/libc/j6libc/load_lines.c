/* _PDCLIB_load_lines( FILE *, size_t )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdlib.h>

char * _PDCLIB_load_lines( FILE * fh, size_t lines )
{
    size_t required = 0;
    long pos = ftell( fh );
    char * rc = NULL;
    int c;

    /* Count the number of characters */
    while ( lines && ( c = fgetc( fh ) ) != EOF )
    {
        if ( c == '\n' )
        {
            --lines;
        }

        ++required;
    }

    if ( ! feof( fh ) )
    {
        if ( ( rc = malloc( required ) ) != NULL )
        {
            size_t i;

            fseek( fh, pos, SEEK_SET );
            fread( rc, 1, required, fh );

            for ( i = 0; i < required; ++i )
            {
                if ( rc[ i ] == '\n' )
                {
                    rc[ i ] = '\0';
                }
            }
        }
    }

    return rc;
}
