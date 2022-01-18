#include <j6/errors.h>
#include <j6/types.h>

#include "logger.h"
#include "objects/endpoint.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
endpoint_create(j6_handle_t *self)
{
    construct_handle<endpoint>(self);
    return j6_status_ok;
}

j6_status_t
endpoint_send(endpoint *self, uint64_t tag, const void * data, size_t data_len)
{
    if (tag & j6_tag_system_flag)
        return j6_err_invalid_arg;

    return self->send(tag, data, data_len);
}

j6_status_t
endpoint_receive(endpoint *self, uint64_t * tag, void * data, size_t * data_len, uint64_t timeout)
{
    // Data is marked optional, but we need the length, and if length > 0,
    // data is not optional.
    if (!data_len || (*data_len && !data))
        return j6_err_invalid_arg;

    // Use local variables instead of the passed-in pointers, since
    // they may get filled in when the sender is running, which means
    // a different user VM space would be active.
    j6_tag_t out_tag = j6_tag_invalid;
    size_t out_len = *data_len;
    j6_status_t s = self->receive(&out_tag, data, &out_len, timeout);
    *tag = out_tag;
    *data_len = out_len;
    return s;
}

j6_status_t
endpoint_sendrecv(endpoint *self, uint64_t * tag, void * data, size_t * data_len, uint64_t timeout)
{
    if (*tag & j6_tag_system_flag)
        return j6_err_invalid_arg;

    j6_status_t status = self->send(*tag, data, *data_len);
    if (status != j6_status_ok)
        return status;

    // Use local variables instead of the passed-in pointers, since
    // they may get filled in when the sender is running, which means
    // a different user VM space would be active.
    j6_tag_t out_tag = j6_tag_invalid;
    size_t out_len = *data_len;
    j6_status_t s = self->receive(&out_tag, data, &out_len, timeout);
    *tag = out_tag;
    *data_len = out_len;
    return s;
}

} // namespace syscalls
