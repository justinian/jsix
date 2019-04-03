#include "log.h"
#include "scheduler.h"

namespace syscalls {

void
exit(int64_t status)
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::debug(logs::syscall, "Process %d exiting with code %d", p->pid, status);

	p->exit(status);
	s.schedule();
}

} // namespace syscalls
