#include "j6/types.h"
#include <j6/errors.h>
#include <j6/protocols/vfs.hh>
#include <j6/syscalls.h>
#include <j6/syslog.hh>

#ifndef __j6kernel

namespace j6::proto::vfs {

client::client(j6_handle_t vfs_mb) :
    m_service {vfs_mb}
{
}

client::client(const client& c) :
    m_service {c.m_service}
{
}

inline size_t simple_strlen(const char *s) { size_t n = 0; while (s && *s) s++, n++; return n; }

j6_status_t
client::load_file(char *path, j6_handle_t &vma, size_t &size)
{
    if (!path)
        return j6_err_invalid_arg;

    uint64_t tag = j6_proto_vfs_load;
    size_t handle_count = 0;
    vma = j6_handle_invalid;

    // Always need to send a big enough buffer for a status code
    size_t path_len = simple_strlen(path);
    size_t data_len = path_len;
    char *data = path;
    j6_status_t alternate = 0;
    if (path_len < sizeof(alternate)) {
        data = reinterpret_cast<char*>(alternate);
        for (unsigned i = 0; i < path_len; ++i)
            data[i] = path[i];
        data_len = sizeof(alternate);
    }

    j6_status_t s = j6_mailbox_call(m_service, &tag,
        data, &data_len, path_len,
        &vma, &handle_count, 1);

    if (s != j6_status_ok)
        return s;

    if (tag == j6_proto_vfs_file && handle_count == 1) {
        size = 0;

        // Get the size into `size`
        s = j6_vma_resize(vma, &size);
        if (s != j6_status_ok)
            return s;

        return j6_status_ok; // handle is already in `vma`
    }

    else if (tag == j6_proto_base_status)
        return *reinterpret_cast<j6_status_t*>(data); // contains a status

    return j6_err_unexpected;
}


j6_status_t
client::get_tag(char *tag, size_t &len)
{
    if (len < sizeof(j6_status_t))
        return j6_err_insufficient;

    uint64_t message_tag = j6_proto_vfs_get_tag;
    size_t handle_count = 0;

    size_t in_len = 0;
    j6_status_t s = j6_mailbox_call(m_service, &message_tag,
        tag, &in_len, len, nullptr, &handle_count, 0);

    if (s != j6_status_ok)
        return s;

    if (message_tag == j6_proto_base_status)
        return *reinterpret_cast<j6_status_t*>(tag); // contains a status

    if (message_tag != j6_proto_vfs_tag)
        return j6_err_unexpected;

    len = in_len;
    return j6_status_ok; // data is now in `tag` and `len`
}

} // namespace j6::proto::vfs
#endif // __j6kernel
