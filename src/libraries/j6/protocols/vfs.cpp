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

inline size_t simple_strlen(const char *s) { size_t n = 0; while (s && *s) s++, n++; return n; }

j6_status_t
client::load_file(char *path, j6_handle_t &vma)
{
    if (!path)
        return j6_err_invalid_arg;

    uint64_t tag = j6_proto_vfs_load;
    size_t handle_count = 1;
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
        &vma, &handle_count);

    if (s != j6_status_ok)
        return s;

    if (tag == j6_proto_vfs_file)
        return j6_status_ok; // handle is already in `vma`

    else if (tag == j6_proto_base_status)
        return *reinterpret_cast<j6_status_t*>(data); // contains a status

    return j6_err_unexpected;
}


} // namespace j6::proto::vfs
#endif // __j6kernel
