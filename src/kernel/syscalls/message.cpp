#include "log.h"
#include "scheduler.h"

namespace syscalls {

void
message(const char *message)
{
	auto &s = scheduler::get();
	auto *p = s.current();
	log::info(logs::syscall, "Message[%d]: %s", p->pid, message);
}

} // namespace syscalls
