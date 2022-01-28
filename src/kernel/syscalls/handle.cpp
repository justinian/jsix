#include <j6/errors.h>
#include <j6/types.h>

#include "objects/process.h"

using namespace obj;

namespace syscalls {

j6_status_t
handle_list(j6_handle_t *handles, size_t *handles_len)
{
    if (!handles_len || (*handles_len && !handles))
        return j6_err_invalid_arg;

    process &p = process::current();
    size_t requested = *handles_len;

    *handles_len = p.list_handles(handles, requested);

    if (*handles_len < requested)
        return j6_err_insufficient;

    return j6_status_ok;
}

} // namespace syscalls
