#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "syscalls/helpers.h"

namespace syscalls {

j6_status_t
thread_create(j6_handle_t *handle, j6_handle_t proc, uintptr_t stack_top, uintptr_t entrypoint)
{
    thread &parent_th = thread::current();
    process &parent_pr = parent_th.parent();

    process *owner = get_handle<process>(proc);
    if (!owner) return j6_err_invalid_arg;

    thread *child = owner->create_thread(stack_top);
    child->add_thunk_user(entrypoint);
    *handle = child->self_handle();
    child->clear_state(thread::state::loading);
    child->set_state(thread::state::ready);

    log::debug(logs::task, "Thread %llx:%llx spawned new thread %llx:%llx",
        parent_pr.koid(), parent_th.koid(), owner->koid(), child->koid());

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
thread_kill(j6_handle_t handle)
{
    thread *th = get_handle<thread>(handle);
    if (!th)
        return j6_err_invalid_arg;

    log::debug(logs::task, "Killing thread %llx", th->koid());
    th->exit(-1);
    return j6_status_ok;
}

j6_status_t
thread_pause()
{
    thread &th = thread::current();
    th.wait_on_signals(-1ull);
    return j6_status_ok;
}

j6_status_t
thread_sleep(uint64_t til)
{
    thread &th = thread::current();
    log::debug(logs::task, "Thread %llx sleeping until %llu", th.koid(), til);

    th.wait_on_time(til);
    return j6_status_ok;
}

} // namespace syscalls
