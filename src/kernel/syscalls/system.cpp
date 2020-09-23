#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/thread.h"

namespace syscalls {

j6_status_t
system_log(const char *message)
{
	if (message == nullptr)
		return j6_err_invalid_arg;

	thread &th = thread::current();
	log::info(logs::syscall, "Message[%llx]: %s", th.koid(), message);
	return j6_status_ok;
}

j6_status_t
system_noop()
{
	thread &th = thread::current();
	log::debug(logs::syscall, "Thread %llx called noop syscall.", th.koid());
	return j6_status_ok;
}

} // namespace syscalls
