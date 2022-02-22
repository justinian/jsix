#include <j6/errors.h>

#include "clock.h"
#include "objects/event.h"
#include "objects/thread.h"
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
    self->signal(signals);
    return j6_status_ok;
}

j6_status_t
event_wait(event *self, j6_signal_t *signals, uint64_t timeout)
{
    thread& t = thread::current();

    if (timeout) {
        timeout += clock::get().value();
        t.set_wake_timeout(timeout);
    }

    *signals = self->wait();
    return j6_status_ok;
}


} // namespace syscalls
