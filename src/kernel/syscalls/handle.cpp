#include <j6/errors.h>
#include <j6/types.h>

#include "objects/process.h"
#include "syscalls/helpers.h"

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

j6_status_t
handle_clone(j6_handle_t orig, j6_handle_t *clone, uint32_t mask)
{
    handle *orig_handle = get_handle<kobject>(orig);
    if (!orig_handle)
        return j6_err_invalid_arg;

    process &p = process::current();
    *clone = p.add_handle(
            orig_handle->object,
            orig_handle->caps() & mask);

    return j6_status_ok;
}

} // namespace syscalls
