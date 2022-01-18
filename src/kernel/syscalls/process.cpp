#include <j6/errors.h>
#include <j6/types.h>

#include "logger.h"
#include "objects/process.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
process_create(j6_handle_t *self)
{
    process *p = construct_handle<process>(self);
    log::debug(logs::task, "Process %llx created", p->koid());
    return j6_status_ok;
}

j6_status_t
process_kill(process *self)
{
    process &p = process::current();

    log::debug(logs::task, "Process %llx killed by process %llx", self->koid(), p.koid());
    self->exit(-1u);

    return j6_status_ok;
}

j6_status_t
process_exit(int32_t status)
{
    process &p = process::current();
    log::debug(logs::task, "Process %llx exiting with code %d", p.koid(), status);

    p.exit(status);

    log::error(logs::task, "returned to exit syscall");
    return j6_err_unexpected;
}

j6_status_t
process_give_handle(process *self, j6_handle_t target, j6_handle_t *received)
{
    handle *target_handle = get_handle<kobject>(target);
    j6_handle_t out = self->add_handle(target_handle->object, target_handle->caps);

    if (received)
        *received = out;
    return j6_status_ok;
}

} // namespace syscalls
