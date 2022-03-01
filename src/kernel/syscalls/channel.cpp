#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/types.h>
#include <util/counted.h>

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
channel_send(channel *self, void *data, size_t *data_len)
{
    if (self->closed())
        return j6_status_closed;

    const util::buffer buffer {data, *data_len};
    *data_len = self->enqueue(buffer);

    return j6_status_ok;
}

j6_status_t
channel_receive(channel *self, void *data, size_t *data_len, uint64_t flags)
{
    if (self->closed())
        return j6_status_closed;

    util::buffer buffer {data, *data_len};

    const bool block = flags & j6_channel_block;
    *data_len = self->dequeue(buffer, block);

    return j6_status_ok;
}

j6_status_t
channel_close(channel *self)
{
    if (self->closed())
        return j6_status_closed;

    self->close();
    return j6_status_ok;
}

} // namespace syscalls
