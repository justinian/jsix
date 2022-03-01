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
thread_create(j6_handle_t *self, process *proc, uintptr_t stack_top, uintptr_t entrypoint)
{
    thread &parent_th = thread::current();
    process &parent_pr = parent_th.parent();

    thread *child = proc->create_thread(stack_top);
    child->add_thunk_user(entrypoint);
    *self = child->self_handle();
    child->set_state(thread::state::ready);

    log::debug(logs::task, "Thread %llx:%llx spawned new thread %llx:%llx",
        parent_pr.koid(), parent_th.koid(), proc->koid(), child->koid());

    return j6_status_ok;
}

j6_status_t
thread_exit(int32_t status)
{
    thread &th = thread::current();
    log::debug(logs::task, "Thread %llx exiting with code %d", th.koid(), status);
    th.exit(status);

    log::error(logs::task, "returned to exit syscall");
    return j6_err_unexpected;
}

j6_status_t
thread_kill(thread *self)
{
    log::debug(logs::task, "Killing thread %llx", self->koid());
    self->exit(-1);
    return j6_status_ok;
}

j6_status_t
thread_sleep(uint64_t duration)
{
    thread &th = thread::current();

    uint64_t til = clock::get().value() + duration;

    log::debug(logs::task, "Thread %llx sleeping until %llu", th.koid(), til);
    th.set_wake_timeout(til);
    th.block();
    return j6_status_ok;
}

} // namespace syscalls
