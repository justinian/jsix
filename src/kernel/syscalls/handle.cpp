#include <j6/errors.h>
#include <j6/types.h>

#include "objects/process.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
handle_list(j6_handle_descriptor *handles, size_t *handles_len)
{
    process &p = process::current();
    size_t requested = *handles_len;

    *handles_len = p.list_handles(handles, requested);

    if (*handles_len > requested)
        return j6_err_insufficient;

    return j6_status_ok;
}

j6_status_t
handle_clone(j6_handle_t orig, j6_handle_t *clone, uint32_t mask)
{
    *clone = g_cap_table.derive(orig, mask);
    process &p = process::current();
    p.add_handle(*clone);

    return j6_status_ok;
}

} // namespace syscalls
