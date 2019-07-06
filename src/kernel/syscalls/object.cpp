#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "scheduler.h"

namespace syscalls {

j6_status_t
object_noop()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::debug(logs::syscall, "Process %d called noop syscall.", p->pid);
	return j6_status_ok;
}

j6_status_t
object_wait(j6_handle_t, j6_signal_t, j6_signal_t*)
{
	return j6_err_nyi;
}

} // namespace syscalls
