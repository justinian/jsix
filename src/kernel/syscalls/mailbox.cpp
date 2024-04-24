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
        size_t data_size,
        j6_handle_t *in_handles,
        size_t *handles_count,
        size_t handles_size)
{
    thread &cur = thread::current();

    util::buffer data {in_data, *data_len};
    util::counted<j6_handle_t> handles {in_handles, *handles_count};

    ipc::message_ptr message = new ipc::message {*tag, data, handles};
    cur.set_message_data(util::move(message));

    j6_status_t s = self->call();
    if (s != j6_status_ok)
        return s;

    message = cur.get_message_data();
    util::counted<j6_handle_t> msg_handles = message->handles();
    util::buffer msg_data = message->data();

    if (message->handle_count) {
        for (unsigned i = 0; i < msg_handles.count; ++i)
            process::current().add_handle(msg_handles[i]);
    }

    *tag = message->tag;
    *data_len = data_size > msg_data.count ? msg_data.count : data_size;
    memcpy(in_data, msg_data.pointer, *data_len);

    *handles_count = handles_size > msg_handles.count ? msg_handles.count : handles_size;
    memcpy(in_handles, msg_handles.pointer, *handles_count * sizeof(j6_handle_t));

    return j6_status_ok;
}

j6_status_t
mailbox_respond(
        mailbox *self,
        uint64_t *tag,
        void *in_data,
        size_t *data_len,
        size_t data_size,
        j6_handle_t *in_handles,
        size_t *handles_count,
        size_t handles_size,
        uint64_t *reply_tag,
        uint64_t flags)
{
    util::buffer data {in_data, *data_len};
    util::counted<j6_handle_t> handles {in_handles, *handles_count};

    ipc::message_ptr message;

    if (*reply_tag) {
        message = new ipc::message {*tag, data, handles};
        j6_status_t s = self->reply(*reply_tag, util::move(message));
        if (s != j6_status_ok)
            return s;
    }

    bool block = flags & j6_flag_block;
    j6_status_t s = self->receive(message, *reply_tag, block);
    if (s != j6_status_ok)
        return s;

    util::counted<j6_handle_t> msg_handles = message->handles();
    util::buffer msg_data = message->data();

    if (msg_handles) {
        for (unsigned i = 0; i < msg_handles.count; ++i)
            process::current().add_handle(msg_handles[i]);
    }

    *tag = message->tag;
    *data_len = data_size > msg_data.count ? msg_data.count : data_size;
    memcpy(in_data, msg_data.pointer, *data_len);

    *handles_count = handles_size > msg_handles.count ? msg_handles.count : handles_size;
    memcpy(in_handles, msg_handles.pointer, *handles_count * sizeof(j6_handle_t));

    return j6_status_ok;
}


} // namespace syscalls
