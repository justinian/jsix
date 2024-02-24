#include <stdint.h>
#include <j6/flags.h>
#include <j6/errors.h>
#include <j6/protocols/vfs.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>

#include "j6romfs.h"
#include "initfs.h"

static uint64_t initfs_running = 1;
static constexpr size_t buffer_size = 2048;

j6_status_t
handle_load_request(j6romfs::fs &fs, const char *path, j6_handle_t &vma)
{
    const j6romfs::inode *in = fs.lookup_inode(path);
    if (!in) {
        vma = j6_handle_invalid;
        return j6_status_ok;
    }

    uintptr_t load_addr = 0;
    j6_vma_create_map(&vma, in->size, &load_addr, j6_vm_flag_write);
    util::buffer dest = util::buffer::from(load_addr, in->size);
    fs.load_inode_data(in, dest);
    j6_vma_unmap(vma, 0);

    return j6_status_ok;
}

void
sanitize(char *s, size_t len)
{
    if (len >= buffer_size) len--;
    for (size_t i = 0; i < len; ++i)
        if (!s || !*s) return;
    s[len] = 0;
}

void
initfs_start(j6romfs::fs &fs, j6_handle_t mb)
{
    uint64_t tag = 0;
    
    char *buffer = new char [buffer_size];
    size_t out_len = buffer_size;

    uint64_t reply_tag = 0;

    size_t handles_count = 1;
    j6_handle_t give_handle = j6_handle_invalid;
    uint64_t proto_id;

    j6_status_t s = j6_mailbox_respond(mb, &tag,
            buffer, &out_len, 0,
            &give_handle, &handles_count,
            &reply_tag, j6_flag_block);

    while (initfs_running) {
        if (s != j6_status_ok) {
            j6::syslog(j6::logs::srv, j6::log_level::error, "initfs: error in j6_mailbox_respond: %x", s);
            return;
        }

        size_t data_out = 0;

        switch (tag) {
        case j6_proto_vfs_load:
            sanitize(buffer, out_len);
            s = handle_load_request(fs, buffer, give_handle);
            if (s != j6_status_ok) {
                tag = j6_proto_base_status;
                *reinterpret_cast<j6_status_t*>(buffer) = s;
                data_out = sizeof(j6_status_t);
                break;
            }
            tag = j6_proto_vfs_file;
            break;

        default:
            tag = j6_proto_base_status;
            *reinterpret_cast<j6_status_t*>(buffer) = j6_err_invalid_arg;
            data_out = sizeof(j6_status_t);
            give_handle = j6_handle_invalid;
        }

        out_len = buffer_size;
        s = j6_mailbox_respond(mb, &tag,
                buffer, &out_len, data_out,
                &give_handle, &handles_count,
                &reply_tag, j6_flag_block);
    }
}
