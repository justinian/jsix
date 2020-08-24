#include <stdarg.h>
#include <limits.h>
#include <float.h>

#include "_PDCLIB_test.h"

typedef int (*intfunc_t)( void );

enum tag_t
{
    TAG_END,
    TAG_INT,
    TAG_LONG,
    TAG_LLONG,
    TAG_DBL,
    TAG_LDBL,
    TAG_INTPTR,
    TAG_LDBLPTR,
    TAG_FUNCPTR
};

static int dummy( void )
{
    return INT_MAX;
}

static int test( enum tag_t s, ... )
{
    enum tag_t tag = s;
    va_list ap;
    va_start( ap, s );
    for (;;)
    {
        switch ( tag )
        {
            case TAG_INT: if( va_arg( ap, int ) != INT_MAX ) return 0; break;
            case TAG_LONG: if( va_arg( ap, long ) != LONG_MAX ) return 0; break;
            case TAG_LLONG: if( va_arg( ap, long long ) != LLONG_MAX ) return 0; break;
            case TAG_DBL: if( va_arg( ap, double ) != DBL_MAX ) return 0; break;
            case TAG_LDBL: if( va_arg( ap, long double ) != LDBL_MAX ) return 0; break;
            case TAG_INTPTR: if( *( va_arg( ap, int * ) ) != INT_MAX ) return 0; break;
            case TAG_LDBLPTR: if( *( va_arg( ap, long double * ) ) != LDBL_MAX ) return 0; break;
            case TAG_FUNCPTR: if( va_arg( ap, intfunc_t ) != dummy ) return 0; break;
            case TAG_END: va_end( ap ); return 1;
        }
		tag = va_arg( ap, enum tag_t );
    }
}

START_TEST( stdarg )
{
    int x = INT_MAX;
    long double d = LDBL_MAX;
    TESTCASE( test(TAG_END) );
    TESTCASE( test(TAG_INT, INT_MAX, TAG_END) );
    TESTCASE( test(TAG_LONG, LONG_MAX, TAG_LLONG, LLONG_MAX, TAG_END) );
    TESTCASE( test(TAG_DBL, DBL_MAX, TAG_LDBL, LDBL_MAX, TAG_END) );
    TESTCASE( test(TAG_INTPTR, &x, TAG_LDBLPTR, &d, TAG_FUNCPTR, dummy, TAG_END) );
}
END_TEST


START_SUITE( stdarg )
{
	RUN_TEST( stdarg );
}
END_SUITE
