#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/endpoint.h"
#include "syscalls/helpers.h"

namespace syscalls {

j6_status_t
endpoint_create(j6_handle_t *handle)
{
	construct_handle<endpoint>(handle);
	return j6_status_ok;
}

j6_status_t
endpoint_close(j6_handle_t handle)
{
	endpoint *e = remove_handle<endpoint>(handle);
	if (!e) return j6_err_invalid_arg;
	e->close();
	return j6_status_ok;
}

j6_status_t
endpoint_send(j6_handle_t handle, size_t len, void *data)
{
	endpoint *e = get_handle<endpoint>(handle);
	if (!e) return j6_err_invalid_arg;
	return e->send(len, data);
}

j6_status_t
endpoint_receive(j6_handle_t handle, size_t *len, void *data)
{
	endpoint *e = get_handle<endpoint>(handle);
	if (!e) return j6_err_invalid_arg;
	return e->receive(len, data);
}

j6_status_t
endpoint_sendrecv(j6_handle_t handle, size_t *len, void *data)
{
	endpoint *e = get_handle<endpoint>(handle);
	if (!e) return j6_err_invalid_arg;

	j6_status_t status = e->send(*len, data);
	if (status != j6_status_ok)
		return status;

	return e->receive(len, data);
}

} // namespace syscalls
