#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "syscalls/helpers.h"

namespace syscalls {

j6_status_t
process_create(j6_handle_t *handle)
{
    process *child = construct_handle<process>(handle);
    log::debug(logs::task, "Process %llx created", child->koid());
    return j6_status_ok;
}

j6_status_t
process_start(j6_handle_t handle, uintptr_t entrypoint, j6_handle_t *handles, size_t handle_count)
{
    process &p = process::current();
    process *c = get_handle<process>(handle);
    if (handle_count && !handles)
        return j6_err_invalid_arg;

    for (size_t i = 0; i < handle_count; ++i) {
        kobject *o = p.lookup_handle(handles[i]);
        if (o) c->add_handle(o);
    }

    return j6_err_nyi;
}

j6_status_t
process_kill(j6_handle_t handle)
{
    process &p = process::current();
    process *c = get_handle<process>(handle);
    if (!c) return j6_err_invalid_arg;

    log::debug(logs::task, "Process %llx killed by process %llx", c->koid(), p.koid());
    c->exit(-1u);

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

} // namespace syscalls
