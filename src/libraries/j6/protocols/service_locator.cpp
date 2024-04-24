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
client::register_service(uint64_t proto_id, util::counted<j6_handle_t> handles)
{
    uint64_t tag = j6_proto_sl_register;
    size_t data = proto_id;
    size_t data_size = sizeof(proto_id);

    j6_status_t s = j6_mailbox_call(m_service, &tag,
        &data, &data_size, data_size,
        handles.pointer, &handles.count, handles.count);

    if (s != j6_status_ok)
        return s;

    if (tag == j6_proto_base_status)
        return data; // contains a status

    return j6_err_unexpected;
}

j6_status_t
client::lookup_service(uint64_t proto_id, util::counted<j6_handle_t> &handles)
{
    uint64_t tag = j6_proto_sl_find;
    size_t data = proto_id;
    size_t data_size = sizeof(proto_id);
    size_t handles_size = handles.count;
    handles.count = 0;

    j6::syslog(j6::logs::proto, j6::log_level::verbose, "Looking up service for %x", proto_id);
    j6_status_t s = j6_mailbox_call(m_service, &tag,
        &data, &data_size, data_size,
        handles.pointer, &handles.count, handles_size);

    if (s != j6_status_ok) {
        j6::syslog(j6::logs::proto, j6::log_level::error, "Received error %lx trying to call service lookup", s);
        return s;
    }

    if (tag == j6_proto_sl_result)
        return j6_status_ok; // handles are already in `handles`

    else if (tag == j6_proto_base_status) {
        j6::syslog(j6::logs::proto, j6::log_level::warn, "Received status %lx from service lookup", data);
        return data; // contains a status
    }

    return j6_err_unexpected;
}


} // namespace j6::proto::sl
#endif // __j6kernel
