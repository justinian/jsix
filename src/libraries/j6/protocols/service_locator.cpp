#include <j6/errors.h>
#include <j6/protocols/service_locator.hh>
#include <j6/syscalls.h>
#include <j6/syslog.hh>

#ifndef __j6kernel

namespace j6::proto::sl {

client::client(j6_handle_t slp_mb) :
    m_service {slp_mb}
{
}

j6_status_t
client::register_service(uint64_t proto_id, j6_handle_t handle)
{
    uint64_t tag = j6_proto_sl_register;
    size_t handle_count = 1;
    size_t data = proto_id;
    size_t data_size = sizeof(proto_id);

    j6_status_t s = j6_mailbox_call(m_service, &tag,
        &data, &data_size, data_size,
        &handle, &handle_count);

    if (s != j6_status_ok)
        return s;

    if (tag == j6_proto_base_status)
        return data; // contains a status

    return j6_err_unexpected;
}

j6_status_t
client::lookup_service(uint64_t proto_id, j6_handle_t &handle)
{
    uint64_t tag = j6_proto_sl_find;
    size_t handle_count = 1;
    size_t data = proto_id;
    size_t data_size = sizeof(proto_id);
    handle = j6_handle_invalid;

    j6::syslog("Looking up service for %x", proto_id);
    j6_status_t s = j6_mailbox_call(m_service, &tag,
        &data, &data_size, data_size,
        &handle, &handle_count);

    if (s != j6_status_ok)
        return s;

    if (tag == j6_proto_sl_result)
        return j6_status_ok; // handle is already in `handle`

    else if (tag == j6_proto_base_status)
        return data; // contains a status

    return j6_err_unexpected;
}


} // namespace j6::proto::sl
#endif // __j6kernel