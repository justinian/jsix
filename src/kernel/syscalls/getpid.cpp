#include "log.h"
#include "scheduler.h"

namespace syscalls {

pid_t
getpid()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	return p->pid;
}

} // namespace syscalls
