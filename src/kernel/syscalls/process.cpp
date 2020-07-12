#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "scheduler.h"

namespace syscalls {

j6_status_t
process_exit(int64_t status)
{
	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = tcb->thread_data;
	log::debug(logs::syscall, "Thread %llx exiting with code %d", th->koid(), status);

	th->exit(status);
	s.schedule();

	log::error(logs::syscall, "returned to exit syscall");
	return j6_err_unexpected;
}

/*
j6_status_t
process_fork(pid_t *pid)
{
	if (pid == nullptr) {
		return j6_err_invalid_arg;
	}

	auto &s = scheduler::get();
	auto *p = s.current();
	pid_t ppid = p->pid;

	log::debug(logs::syscall, "Process %d calling fork(%016llx)", ppid, pid);

	*pid = p->fork();

	p = s.current();
	log::debug(logs::syscall, "Process %d's fork: returning %d from process %d", ppid, *pid, p->pid);

	return j6_status_ok;
}

j6_status_t
process_getpid(pid_t *pid)
{
	if (pid == nullptr) {
		return j6_err_invalid_arg;
	}

	auto &s = scheduler::get();
	auto *p = s.current();

	*pid = p->pid;
	return j6_status_ok;
}
*/

j6_status_t
process_log(const char *message)
{
	if (message == nullptr) {
		return j6_err_invalid_arg;
	}

	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = tcb->thread_data;
	log::info(logs::syscall, "Message[%llx]: %s", th->koid(), message);
	return j6_status_ok;
}

j6_status_t process_fork(uint32_t *pid) { *pid = 5; return process_log("CALLED FORK"); }
j6_status_t process_getpid(uint32_t *pid) { *pid = 0; return process_log("CALLED GETPID"); }

j6_status_t
process_pause()
{
	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = tcb->thread_data;
	th->wait_on_signals(th, -1ull);
	s.schedule();
	return j6_status_ok;
}

j6_status_t
process_sleep(uint64_t til)
{
	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = tcb->thread_data;
	log::debug(logs::syscall, "Thread %llx sleeping until %llu", th->koid(), til);

	th->wait_on_time(til);
	s.schedule();
	return j6_status_ok;
}

} // namespace syscalls
