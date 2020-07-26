#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "scheduler.h"

namespace syscalls {

j6_status_t
system_log(const char *message)
{
	if (message == nullptr) {
		return j6_err_invalid_arg;
	}

	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = thread::from_tcb(tcb);
	log::info(logs::syscall, "Message[%llx]: %s", th->koid(), message);
	return j6_status_ok;
}

j6_status_t
system_noop()
{
	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = thread::from_tcb(tcb);
	log::debug(logs::syscall, "Thread %llx called noop syscall.", th->koid());
	return j6_status_ok;
}

} // namespace syscalls
