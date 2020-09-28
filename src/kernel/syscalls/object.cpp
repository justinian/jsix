#include "j6/errors.h"
#include "j6/signals.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "objects/thread.h"

namespace syscalls {

j6_status_t
object_wait(j6_handle_t handle, j6_signal_t mask, j6_signal_t *sigs)
{
	thread &th = thread::current();
	process &p = process::current();

	kobject *obj = p.lookup_handle(handle);
	if (!obj)
		return j6_err_invalid_arg;

	j6_signal_t current = obj->signals();
	if ((current & mask) != 0) {
		*sigs = current;
		return j6_status_ok;
	}

	obj->add_blocked_thread(&th);
	th.wait_on_signals(obj, mask);

	j6_status_t result = th.get_wait_result();
	if (result == j6_status_ok) {
		*sigs = th.get_wait_data();
	}
	return result;
}

j6_status_t
object_signal(j6_handle_t handle, j6_signal_t signals)
{
	if ((signals & j6_signal_user_mask) != signals)
		return j6_err_invalid_arg;

	process &p = process::current();
	kobject *obj = p.lookup_handle(handle);
	if (!obj)
		return j6_err_invalid_arg;

	obj->assert_signal(signals);
	return j6_status_ok;
}

} // namespace syscalls
