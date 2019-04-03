
#include "log.h"
#include "scheduler.h"

namespace syscalls {

pid_t
fork()
{
	auto &s = scheduler::get();
	auto *p = s.current();
	pid_t ppid = p->pid;

	log::debug(logs::syscall, "Process %d calling fork()", ppid);

	pid_t pid = p->fork();

	p = s.current();
	log::debug(logs::syscall, "Process %d's fork: returning %d from process %d", ppid, pid, p->pid);

	return pid;
}

} // namespace syscalls
