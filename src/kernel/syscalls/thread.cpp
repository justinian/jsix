#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "objects/thread.h"

namespace syscalls {

j6_status_t
thread_create(void *rip, j6_handle_t *handle)
{
	thread &parent = thread::current();
	process &p = parent.parent();

	thread *child = p.create_thread();
	child->add_thunk_user(reinterpret_cast<uintptr_t>(rip));
	*handle = child->self_handle();
	child->clear_state(thread::state::loading);
	child->set_state(thread::state::ready);

	log::debug(logs::task, "Thread %llx spawned new thread %llx, handle %d",
		parent.koid(), child->koid(), *handle);

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
