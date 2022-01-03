#include <j6/errors.h>
#include <j6/types.h>

#include "objects/channel.h"
#include "syscalls/helpers.h"

namespace syscalls {

j6_status_t
channel_create(j6_handle_t *handle)
{
    construct_handle<channel>(handle);
    return j6_status_ok;
}

j6_status_t
channel_send(j6_handle_t handle, size_t *len, void *data)
{
    channel *c = get_handle<channel>(handle);
    if (!c) return j6_err_invalid_arg;
    return c->enqueue(len, data);
}

j6_status_t
channel_receive(j6_handle_t handle, size_t *len, void *data)
{
    channel *c = get_handle<channel>(handle);
    if (!c) return j6_err_invalid_arg;
    return c->dequeue(len, data);
}

} // namespace syscalls
