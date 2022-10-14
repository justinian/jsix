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
mailbox_call(
        mailbox *self,
        uint64_t *tag,
        uint64_t *subtag,
        j6_handle_t *handle)
{
    thread::message_data &data =
        thread::current().get_message_data();

    data.tag = *tag;
    data.subtag = *subtag;
    data.handle = *handle;

    j6_status_t s = self->call();
    if (s != j6_status_ok)
        return s;

    *tag = data.tag;
    *subtag = data.subtag;
    *handle = data.handle;
    process::current().add_handle(*handle);

    return j6_status_ok;
}

j6_status_t
mailbox_respond(
        mailbox *self,
        uint64_t *tag,
        uint64_t *subtag,
        j6_handle_t *handle,
        uint64_t *reply_tag,
        uint64_t flags)
{
    thread::message_data data { *tag, *subtag, *handle };

    if (*reply_tag) {
        j6_status_t s = self->reply(*reply_tag, data);
        if (s != j6_status_ok)
            return s;
    }

    bool block = flags & j6_mailbox_block;
    j6_status_t s = self->receive(data, *reply_tag, block);
    if (s != j6_status_ok)
        return s;

    *tag = data.tag;
    *subtag = data.subtag;
    *handle = data.handle;
    process::current().add_handle(*handle);

    return j6_status_ok;
}


} // namespace syscalls
