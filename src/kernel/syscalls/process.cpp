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
    log::info(logs::task, "Process <%02lx> created", p->obj_id());
    return j6_status_ok;
}

j6_status_t
process_kill(process *self)
{
    process &p = process::current();

    log::info(logs::task, "Process <%02lx> killed by process <%02lx>", self->obj_id(), p.obj_id());
    self->exit(-1u);

    return j6_status_ok;
}

j6_status_t
process_exit(int32_t status)
{
    process &p = process::current();
    log::info(logs::task, "Process <%02lx> exiting with code %d", p.obj_id(), status);

    p.exit(status);

    log::error(logs::task, "returned to exit syscall");
    return j6_err_unexpected;
}

j6_status_t
process_give_handle(process *self, j6_handle_t target)
{
    self->add_handle(target);
    return j6_status_ok;
}

} // namespace syscalls
