#include "log.h"
#include "scheduler.h"

namespace syscalls {

void
noop()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::debug(logs::syscall, "Process %d called noop syscall.", p->pid);
}

} // namespace syscalls
