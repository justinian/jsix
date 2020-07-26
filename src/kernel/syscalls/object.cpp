#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "scheduler.h"

namespace syscalls {

j6_status_t
object_wait(j6_handle_t handle, j6_signal_t mask, j6_signal_t *sigs)
{
	scheduler &s = scheduler::get();
	thread *th = thread::from_tcb(s.current());
	process &p = th->parent();

	kobject *obj = p.lookup_handle(handle);
	if (!obj)
		return j6_err_invalid_arg;

	obj->add_blocked_thread(th);
	th->wait_on_signals(obj, mask);
	s.schedule();

	j6_status_t result = th->get_wait_result();
	if (result == j6_status_ok) {
		*sigs = th->get_wait_data();
	}
	return result;
}

} // namespace syscalls
