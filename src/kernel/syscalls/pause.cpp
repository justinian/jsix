#include "log.h"
#include "scheduler.h"

namespace syscalls {

void
pause()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	p->wait_on_signal(-1ull);
	s.schedule();
}

} // namespace syscalls
