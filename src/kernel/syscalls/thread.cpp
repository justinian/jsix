#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "scheduler.h"

namespace syscalls {

j6_status_t
thread_koid(j6_koid_t *koid)
{
	if (koid == nullptr) {
		return j6_err_invalid_arg;
	}

	TCB *tcb = scheduler::get().current();
	*koid =  thread::from_tcb(tcb)->koid();
	return j6_status_ok;
}

j6_status_t
thread_create(void *rip, j6_handle_t *handle)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	thread *child = p.create_thread(scheduler::default_priority);
	child->add_thunk_user(reinterpret_cast<uintptr_t>(rip));
	*handle = p.add_handle(child);
	s.add_thread(child->tcb());

	log::debug(logs::syscall, "Thread %llx spawned new thread %llx, handle %d",
		parent->koid(), child->koid(), *handle);

	return j6_status_ok;
}

j6_status_t
thread_exit(int64_t status)
{
	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = thread::from_tcb(tcb);
	log::debug(logs::syscall, "Thread %llx exiting with code %d", th->koid(), status);

	th->exit(status);
	s.schedule();

	log::error(logs::syscall, "returned to exit syscall");
	return j6_err_unexpected;
}

j6_status_t
thread_pause()
{
	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = thread::from_tcb(tcb);
	th->wait_on_signals(th, -1ull);
	s.schedule();
	return j6_status_ok;
}

j6_status_t
thread_sleep(uint64_t til)
{
	auto &s = scheduler::get();
	TCB *tcb = s.current();
	thread *th = thread::from_tcb(tcb);
	log::debug(logs::syscall, "Thread %llx sleeping until %llu", th->koid(), til);

	th->wait_on_time(til);
	s.schedule();
	return j6_status_ok;
}

} // namespace syscalls
