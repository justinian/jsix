#include <j6/errors.h>
#include <j6/flags.h>
#include <util/util.h>

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
prep_send(
        mailbox::message *msg,
        uint64_t tag,
        uint64_t badge,
        const void *data,
        size_t data_len,
        const j6_handle_t *handles,
        size_t handle_count)
{
    if (!msg ||
        data_len > mailbox::max_data_length ||
        handle_count > mailbox::max_handle_count)
        return j6_err_invalid_arg;

    msg->tag = tag;
    msg->badge = badge;

    msg->handle_count = handle_count;
    for (unsigned i = 0; i < handle_count; ++i) {
        handle const *h = get_handle<kobject>(handles[i]);
        if (!h)
            return j6_err_invalid_arg;
        msg->handles[i] = *h;
    }

    msg->data_len = data_len;
    memcpy(msg->data, data, data_len);
    return j6_status_ok;
}

void
prep_receive(
        mailbox::message *msg,
        uint64_t *tag,
        uint64_t *badge,
        uint16_t *reply_tag,
        void *data,
        size_t *data_len,
        j6_handle_t *handles,
        size_t *handle_count)
{
    if (tag) *tag = msg->tag;
    if (badge) *badge = msg->badge;
    if (reply_tag) *reply_tag = msg->reply_tag;

    *data_len = msg->data_len;
    memcpy(data, msg->data, msg->data_len);

    *handle_count = msg->handle_count;
    process &proc = process::current();
    for (size_t i = 0; i < msg->handle_count; ++i) {
        handles[i] = proc.add_handle(msg->handles[i]);
        msg->handles[i] = {};
    }
}

j6_status_t
mailbox_send(
        handle *self_handle,
        uint64_t tag,
        const void * data,
        size_t data_len,
        j6_handle_t * handles,
        size_t handle_count)
{
    mailbox *self = self_handle->as<mailbox>();
    mailbox::message *msg = new mailbox::message;

    j6_status_t s = prep_send(msg,
            tag, self_handle->badge,
            data, data_len,
            handles, handle_count);

    if (s != j6_status_ok) {
        delete msg;
        return s;
    }

    self->send(msg);
    return j6_status_ok;
}


j6_status_t
mailbox_receive(
        mailbox *self,
        uint64_t * tag,
        void * data,
        size_t * data_len,
        j6_handle_t * handles,
        size_t * handle_count,
        uint16_t * reply_tag,
        uint64_t * badge,
        uint64_t flags)
{
    if (*data_len < mailbox::max_data_length ||
        *handle_count < mailbox::max_handle_count)
        return j6_err_insufficient;

    mailbox::message *msg = nullptr;

    bool block = flags & j6_mailbox_block;
    if (!self->receive(msg, block)) {
        // No message received
        return self->closed() ? j6_status_closed :
               block ? j6_status_would_block :
               j6_err_unexpected;
    }

    prep_receive(msg,
            tag, badge, reply_tag,
            data, data_len,
            handles, handle_count);

    if (*reply_tag == 0)
        delete msg;
    return j6_status_ok;
}


j6_status_t
mailbox_call(
        handle *self_handle,
        uint64_t * tag,
        void * data,
        size_t * data_len,
        j6_handle_t * handles,
        size_t * handle_count)
{
    mailbox *self = self_handle->as<mailbox>();
    mailbox::message *msg = new mailbox::message;

    j6_status_t s = prep_send(msg,
            *tag, self_handle->badge,
            data, *data_len,
            handles, *handle_count);

    if (s != j6_status_ok) {
        delete msg;
        return s;
    }

    if (!self->call(msg)) {
        delete msg;
        return self->closed() ? j6_status_closed :
               j6_err_unexpected;
    }

    prep_receive(msg,
            tag, nullptr, nullptr,
            data, data_len,
            handles, handle_count);

    delete msg;
    return j6_status_ok;
}


j6_status_t
mailbox_respond(
        mailbox *self,
        uint64_t tag,
        const void * data,
        size_t data_len,
        j6_handle_t * handles,
        size_t handle_count,
        uint16_t reply_tag)
{
    mailbox::replyer reply = self->reply(reply_tag);
    if (!reply.valid())
        return j6_err_invalid_arg;

    j6_status_t s = prep_send(reply.msg, tag, 0,
            data, data_len,
            handles, handle_count);

    if (s != j6_status_ok) {
        reply.error(s);
        return s;
    }

    return j6_status_ok;
}


j6_status_t
mailbox_respond_receive(
        mailbox *self,
        uint64_t * tag,
        void * data,
        size_t * data_len,
        size_t data_in,
        j6_handle_t * handles,
        size_t * handle_count,
        size_t handles_in,
        uint16_t * reply_tag,
        uint64_t * badge,
        uint64_t flags)
{
    j6_status_t s = mailbox_respond(self, *tag, data, data_in, handles, handles_in, *reply_tag);
    if (s != j6_status_ok)
        return s;

    s = mailbox_receive(self, tag, data, data_len, handles, handle_count, reply_tag, badge, flags);
    if (s != j6_status_ok)
        return s;

    return j6_status_ok;
}



} // namespace syscalls
