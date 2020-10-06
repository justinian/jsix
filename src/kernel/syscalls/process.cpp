#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"

namespace syscalls {

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
