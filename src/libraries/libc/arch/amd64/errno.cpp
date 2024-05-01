#include <arch/amd64/errno.h>

static int errno_value = 0;

int *
__errno_location()
{
    // TODO: thread-local errno
    return &errno_value;
}
