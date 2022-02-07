#include <assert.h>
#include <stdlib.h>

noreturn void
__assert_fail( const char *, const char *, unsigned, const char * )
{
    // TODO: display message
    abort();
}
