#include <j6/errors.h>
#include <j6/signals.h>

#include "objects/event.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
event_create(j6_handle_t *self)
{
    construct_handle<event>(self);
    return j6_status_ok;
}

j6_status_t
event_signal(event *self, j6_signal_t signals)
{
    if (signals & j6_signal_global_mask)
        return j6_err_invalid_arg;

    self->assert_signal(signals);
    return j6_status_ok;
}

j6_status_t
event_clear(event *self, j6_signal_t mask)
{
    self->deassert_signal(~mask);
    return j6_status_ok;
}


} // namespace syscalls
