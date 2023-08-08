#include <j6/errors.h>
#include <j6/flags.h>
#include <util/counted.h>
#include <util/util.h>

#include "ipc_message.h"
#include "objects/mailbox.h"
#include "objects/thread.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
mailbox_create(j6_handle_t *self)
{
    construct_handle<mailbox>(self);
    return j6_status_ok;
}

j6_status_t
mailbox_close(mailbox *self)
{
    if (self->closed())
        return j6_status_closed;

    self->close();
    return j6_status_ok;
}

j6_status_t
mailbox_call(
        mailbox *self,
        uint64_t *tag,
        void *in_data,
        size_t *data_len,
        size_t data_in_len,
        j6_handle_t *in_handles,
        size_t *handles_count)
{
    thread &cur = thread::current();

    util::buffer data {in_data, data_in_len};
    util::counted<j6_handle_t> handles {in_handles, *handles_count};

    ipc::message message(*tag, data, handles);
    cur.set_message_data(util::move(message));

    j6_status_t s = self->call();
    if (s != j6_status_ok)
        return s;

    message = cur.get_message_data();

    if (message.handles) {
        for (unsigned i = 0; i < message.handles.count; ++i)
            process::current().add_handle(message.handles[i]);
    }

    *tag = message.tag;
    *data_len = *data_len > message.data.count ? message.data.count : *data_len;
    memcpy(in_data, message.data.pointer, *data_len);

    size_t handles_min = *handles_count > message.handles.count ? message.handles.count : *handles_count;
    *handles_count = handles_min;
    memcpy(in_handles, message.handles.pointer, handles_min * sizeof(j6_handle_t));

    return j6_status_ok;
}

j6_status_t
mailbox_respond(
        mailbox *self,
        uint64_t *tag,
        void *in_data,
        size_t *data_len,
        size_t data_in_len,
        j6_handle_t *in_handles,
        size_t *handles_count,
        uint64_t *reply_tag,
        uint64_t flags)
{
    util::buffer data {in_data, data_in_len};
    util::counted<j6_handle_t> handles {in_handles, *handles_count};

    ipc::message message(*tag, data, handles);

    if (*reply_tag) {
        j6_status_t s = self->reply(*reply_tag, util::move(message));
        if (s != j6_status_ok)
            return s;
    }

    bool block = flags & j6_flag_block;
    j6_status_t s = self->receive(message, *reply_tag, block);
    if (s != j6_status_ok)
        return s;

    if (message.handles) {
        for (unsigned i = 0; i < message.handles.count; ++i)
            process::current().add_handle(message.handles[i]);
    }

    *tag = message.tag;
    *data_len = *data_len > message.data.count ? message.data.count : *data_len;
    memcpy(in_data, message.data.pointer, *data_len);

    size_t handles_min = *handles_count > message.handles.count ? message.handles.count : *handles_count;
    *handles_count = handles_min;
    memcpy(in_handles, message.handles.pointer, handles_min * sizeof(j6_handle_t));

    return j6_status_ok;
}


} // namespace syscalls
