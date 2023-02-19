#include <j6/errors.h>
#include <j6/types.h>

#include "clock.h"
#include "logger.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
thread_create(j6_handle_t *self, process *proc, uintptr_t stack_top, uintptr_t entrypoint, uint64_t arg0, uint64_t arg1)
{
    thread &parent_th = thread::current();
    process &parent_pr = parent_th.parent();

    if (!proc)
        proc = &parent_pr;

    thread *child = proc->create_thread(stack_top);
    child->add_thunk_user(entrypoint, arg0, arg1);

    *self = g_cap_table.create(child, thread::creation_caps);

    child->set_state(thread::state::ready);

    log::verbose(logs::task, "Thread <%02lx:%02lx> spawned new thread <%02lx:%02lx>",
        parent_pr.obj_id(), parent_th.obj_id(), proc->obj_id(), child->obj_id());

    return j6_status_ok;
}

j6_status_t
thread_kill(thread *self)
{
    log::verbose(logs::task, "Killing thread <%02lx:%02lx>", self->parent().obj_id(), self->obj_id());
    self->exit();
    return j6_status_ok;
}

j6_status_t
thread_join(thread *self)
{
    return self->join();
}

j6_status_t
thread_exit()
{
    thread &th = thread::current();
    log::verbose(logs::task, "Thread <%02lx:%02lx> exiting", th.parent().obj_id(), th.obj_id());
    th.exit();

    log::error(logs::task, "returned to exit syscall");
    return j6_err_unexpected;
}

j6_status_t
thread_sleep(uint64_t duration)
{
    thread &th = thread::current();

    uint64_t til = clock::get().value() + duration;

    log::verbose(logs::task, "Thread <%02lx:%02lx> sleeping until %llu", th.parent().obj_id(), th.obj_id(), til);
    th.set_wake_timeout(til);
    th.block();
    return j6_status_ok;
}

} // namespace syscalls
