#include <j6/errors.h>
#include <j6/types.h>

#include "objects/channel.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
channel_create(j6_handle_t *self)
{
    construct_handle<channel>(self);
    return j6_status_ok;
}

j6_status_t
channel_send(channel *self, size_t *len, void *data)
{
    return self->enqueue(len, data);
}

j6_status_t
channel_receive(channel *self, size_t *len, void *data)
{
    return self->dequeue(len, data);
}

} // namespace syscalls
