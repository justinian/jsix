#include "log.h"
#include "scheduler.h"

namespace syscalls {

void
sleep(uint64_t til)
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::debug(logs::syscall, "Process %d sleeping until %d", p->pid, til);

	p->wait_on_time(til);
	s.schedule();
}

} // namespace syscalls
