#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "syscalls/helpers.h"

namespace syscalls {

j6_status_t
process_create(j6_handle_t *handle)
{
	process *child = construct_handle<process>(handle);
	log::debug(logs::syscall, "Process %llx created", child->koid());
	return j6_status_ok;
}

j6_status_t
process_start(j6_handle_t *handle, uintptr_t entrypoint)
{
	return j6_err_nyi;
}

j6_status_t
process_exit(int64_t status)
{
	process &p = process::current();
	log::debug(logs::syscall, "Process %llx exiting with code %d", p.koid(), status);

	p.exit(status);

	log::error(logs::syscall, "returned to exit syscall");
	return j6_err_unexpected;
}

} // namespace syscalls
