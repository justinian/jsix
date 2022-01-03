#include <j6/errors.h>
#include <j6/types.h>

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
endpoint_send(j6_handle_t handle, uint64_t tag, const void * data, size_t data_len)
{
    if (tag & j6_tag_system_flag)
        return j6_err_invalid_arg;

    endpoint *e = get_handle<endpoint>(handle);
    if (!e) return j6_err_invalid_arg;

    return e->send(tag, data, data_len);
}

j6_status_t
endpoint_receive(j6_handle_t handle, uint64_t * tag, void * data, size_t * data_len)
{
    if (!tag || !data_len || (*data_len && !data))
        return j6_err_invalid_arg;

    endpoint *e = get_handle<endpoint>(handle);
    if (!e) return j6_err_invalid_arg;

    j6_tag_t out_tag = j6_tag_invalid;
    size_t out_len = *data_len;
    j6_status_t s = e->receive(&out_tag, data, &out_len);
    *tag = out_tag;
    *data_len = out_len;
    return s;
}

j6_status_t
endpoint_sendrecv(j6_handle_t handle, uint64_t * tag, void * data, size_t * data_len)
{
    if (!tag || (*tag & j6_tag_system_flag))
        return j6_err_invalid_arg;

    endpoint *e = get_handle<endpoint>(handle);
    if (!e) return j6_err_invalid_arg;

    j6_status_t status = e->send(*tag, data, *data_len);
    if (status != j6_status_ok)
        return status;

    j6_tag_t out_tag = j6_tag_invalid;
    size_t out_len = *data_len;
    j6_status_t s = e->receive(&out_tag, data, &out_len);
    *tag = out_tag;
    *data_len = out_len;
    return s;
}

} // namespace syscalls
