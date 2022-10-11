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
        uint64_t subtag,
        const j6_handle_t *handles,
        size_t handle_count)
{
    if (!msg ||
        handle_count > mailbox::max_handle_count)
        return j6_err_invalid_arg;

    msg->tag = tag;
    msg->subtag = subtag;

    msg->handle_count = handle_count;
    memcpy(msg->handles, handles, sizeof(j6_handle_t) * handle_count);

    return j6_status_ok;
}

void
prep_receive(
        mailbox::message *msg,
        uint64_t *tag,
        uint64_t *subtag,
        uint16_t *reply_tag,
        j6_handle_t *handles,
        size_t *handle_count)
{
    if (tag) *tag = msg->tag;
    if (subtag) *subtag = msg->subtag;
    if (reply_tag) *reply_tag = msg->reply_tag;

    *handle_count = msg->handle_count;
    process &proc = process::current();
    for (size_t i = 0; i < msg->handle_count; ++i) {
        proc.add_handle(msg->handles[i]);
        handles[i] = msg->handles[i];
    }
}

j6_status_t
mailbox_send(
        mailbox *self,
        uint64_t tag,
        uint64_t subtag,
        j6_handle_t * handles,
        size_t handle_count)
{
    mailbox::message *msg = new mailbox::message;

    j6_status_t s = prep_send(msg,
            tag, subtag,
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
        uint64_t *tag,
        uint64_t *subtag,
        j6_handle_t *handles,
        size_t *handle_count,
        uint16_t *reply_tag,
        uint64_t flags)
{
    if (*handle_count < mailbox::max_handle_count)
        return j6_err_insufficient;

    mailbox::message *msg = nullptr;

    bool block = flags & j6_mailbox_block;
    if (!self->receive(msg, block)) {
        // No message received
        return self->closed() ? j6_status_closed :
               !block ? j6_status_would_block :
               j6_err_unexpected;
    }

    prep_receive(msg,
            tag, subtag, reply_tag,
            handles, handle_count);

    if (*reply_tag == 0)
        delete msg;
    return j6_status_ok;
}


j6_status_t
mailbox_call(
        mailbox *self,
        uint64_t *tag,
        uint64_t *subtag,
        j6_handle_t *handles,
        size_t *handle_count)
{
    mailbox::message *msg = new mailbox::message;

    j6_status_t s = prep_send(msg,
            *tag, *subtag,
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
            tag, subtag, 0,
            handles, handle_count);

    delete msg;
    return j6_status_ok;
}


j6_status_t
mailbox_respond(
        mailbox *self,
        uint64_t tag,
        uint64_t subtag,
        j6_handle_t * handles,
        size_t handle_count,
        uint16_t reply_tag)
{
    mailbox::replyer reply = self->reply(reply_tag);
    if (!reply.valid())
        return j6_err_invalid_arg;

    j6_status_t s = prep_send(reply.msg, tag, subtag,
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
        uint64_t *tag,
        uint64_t *subtag,
        j6_handle_t *handles,
        size_t *handle_count,
        size_t handles_in,
        uint16_t *reply_tag,
        uint64_t flags)
{
    j6_status_t s = mailbox_respond(self, *tag, *subtag, handles, handles_in, *reply_tag);
    if (s != j6_status_ok)
        return s;

    s = mailbox_receive(self, tag, subtag, handles, handle_count, reply_tag, flags);
    if (s != j6_status_ok)
        return s;

    return j6_status_ok;
}



} // namespace syscalls
