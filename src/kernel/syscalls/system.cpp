#include "j6/errors.h"
#include "j6/types.h"

#include "device_manager.h"
#include "log.h"
#include "objects/endpoint.h"
#include "objects/thread.h"
#include "syscalls/helpers.h"

extern log::logger &g_logger;

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

j6_status_t
system_get_log(j6_handle_t sys, char *buffer, size_t *size)
{
	*size = g_logger.get_entry(buffer, *size);
	return j6_status_ok;
}

j6_status_t
system_bind_irq(j6_handle_t sys, j6_handle_t endp, unsigned irq)
{
	// TODO: check capabilities on sys handle
	endpoint *e = get_handle<endpoint>(endp);
	if (!e) return j6_err_invalid_arg;

	if (device_manager::get().bind_irq(irq, e))
		return j6_status_ok;

	return j6_err_invalid_arg;
}

} // namespace syscalls
