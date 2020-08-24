/* _PDCLIB_load_lc_monetary( const char *, const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "j6libc/int.h"

struct _PDCLIB_lc_lconv_monetary_t * _PDCLIB_load_lc_monetary( const char * path, const char * locale )
{
    struct _PDCLIB_lc_lconv_monetary_t * rc = NULL;
    const char * extension = "_monetary.dat";
    char * file = malloc( strlen( path ) + strlen( locale ) + strlen( extension ) + 1 );

    if ( file )
    {
        FILE * fh;

        strcpy( file, path );
        strcat( file, locale );
        strcat( file, extension );

        if ( ( fh = fopen( file, "rb" ) ) != NULL )
        {
            if ( ( rc = malloc( sizeof( struct _PDCLIB_lc_lconv_monetary_t ) ) ) != NULL )
            {
                char buffer[ 14 ];
                char * data = _PDCLIB_load_lines( fh, 7 );

                if ( data != NULL )
                {
                    if ( fread( buffer, 1, 14, fh ) == 14 )
                    {
                        rc->mon_decimal_point = data;
                        data += strlen( data ) + 1;
                        rc->mon_thousands_sep = data;
                        data += strlen( data ) + 1;
                        rc->mon_grouping = data;
                        data += strlen( data ) + 1;
                        rc->positive_sign = data;
                        data += strlen( data ) + 1;
                        rc->negative_sign = data;
                        data += strlen( data ) + 1;
                        rc->currency_symbol = data;
                        data += strlen( data ) + 1;
                        rc->int_curr_symbol = data;

                        rc->frac_digits = buffer[ 0 ];
                        rc->p_cs_precedes = buffer[ 1 ];
                        rc->n_cs_precedes = buffer[ 2 ];
                        rc->p_sep_by_space = buffer[ 3 ];
                        rc->n_sep_by_space = buffer[ 4 ];
                        rc->p_sign_posn = buffer[ 5 ];
                        rc->n_sign_posn = buffer[ 6 ];
                        rc->int_frac_digits = buffer[ 7 ];
                        rc->int_p_cs_precedes = buffer[ 8 ];
                        rc->int_n_cs_precedes = buffer[ 9 ];
                        rc->int_p_sep_by_space = buffer[ 10 ];
                        rc->int_n_sep_by_space = buffer[ 11 ];
                        rc->int_p_sign_posn = buffer[ 12 ];
                        rc->int_n_sign_posn= buffer[ 13 ];
                    }
                    else
                    {
                        free( data );
                        free( rc );
                        rc = NULL;
                    }
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
