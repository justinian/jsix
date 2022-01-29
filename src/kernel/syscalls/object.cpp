#include <j6/errors.h>
#include <j6/signals.h>
#include <j6/types.h>
#include <util/vector.h>

#include "assert.h"
#include "logger.h"
#include "objects/thread.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
object_koid(kobject *self, j6_koid_t *koid)
{
    *koid = self->koid();
    return j6_status_ok;
}

j6_status_t
object_wait(kobject *self, j6_signal_t mask, j6_signal_t *sigs)
{
    j6_signal_t current = self->signals();
    if ((current & mask) != 0) {
        *sigs = current;
        return j6_status_ok;
    }

    thread &th = thread::current();
    self->add_blocked_thread(&th);
    th.wait_on_signals(mask);

    j6_status_t result = th.get_wait_result();
    if (result == j6_status_ok) {
        *sigs = th.get_wait_data();
    }
    return result;
}

j6_status_t
object_wait_many(j6_handle_t * handles, size_t handles_count, uint64_t mask, j6_handle_t * woken, uint64_t * signals)
{
    util::vector<kobject*> objects {uint32_t(handles_count)};

    for (unsigned i = 0; i < handles_count; ++i) {
        j6_handle_t hid = handles[i];
        if (hid == j6_handle_invalid)
            continue;

        handle *h = get_handle<kobject>(hid);
        if (!h || !h->object)
            return j6_err_invalid_arg;
        kobject *obj = h->object;

        j6_signal_t current = obj->signals();
        if ((current & mask) != 0) {
            *signals = current;
            *woken = hid;
            return j6_status_ok;
        }

        objects.append(obj);
    }

    thread &th = thread::current();
    for (auto *obj : objects)
        obj->add_blocked_thread(&th);

    th.wait_on_signals(mask);

    j6_status_t result = th.get_wait_result();
    if (result != j6_status_ok)
        return result;

    *woken = j6_handle_invalid;
    *signals = th.get_wait_data();
    j6_koid_t koid = th.get_wait_object();
    for (unsigned i = 0; i < handles_count; ++i) {
        if (koid == objects[i]->koid())
            *woken = handles[i];
        else
            objects[i]->remove_blocked_thread(&th);
    }

    kassert(*woken != j6_handle_invalid,
        "Somehow woke on a handle that was not waited on");
    return j6_status_ok;
}

j6_status_t
object_signal(kobject *self, j6_signal_t signals)
{
    if ((signals & j6_signal_user_mask) != signals)
        return j6_err_invalid_arg;

    self->assert_signal(signals);
    return j6_status_ok;
}

j6_status_t
object_close(kobject *self)
{
    self->close();
    return j6_status_ok;
}

} // namespace syscalls
