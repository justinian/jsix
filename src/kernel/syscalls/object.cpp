#include <j6/errors.h>
#include <j6/signals.h>
#include <j6/types.h>
#include <util/vector.h>

#include "assert.h"
#include "logger.h"
#include "objects/thread.h"
#include "syscalls/helpers.h"

namespace syscalls {

j6_status_t
kobject_koid(j6_handle_t handle, j6_koid_t *koid)
{
    if (koid == nullptr)
        return j6_err_invalid_arg;

    kobject *obj = get_handle<kobject>(handle);
    if (!obj)
        return j6_err_invalid_arg;

    *koid = obj->koid();
    return j6_status_ok;
}

j6_status_t
kobject_wait(j6_handle_t handle, j6_signal_t mask, j6_signal_t *sigs)
{
    kobject *obj = get_handle<kobject>(handle);
    if (!obj)
        return j6_err_invalid_arg;

    j6_signal_t current = obj->signals();
    if ((current & mask) != 0) {
        *sigs = current;
        return j6_status_ok;
    }

    thread &th = thread::current();
    obj->add_blocked_thread(&th);
    th.wait_on_signals(mask);

    j6_status_t result = th.get_wait_result();
    if (result == j6_status_ok) {
        *sigs = th.get_wait_data();
    }
    return result;
}

j6_status_t
kobject_wait_many(j6_handle_t * handles, size_t handles_count, uint64_t mask, j6_handle_t * handle, uint64_t * signals)
{
    util::vector<kobject*> objects {uint32_t(handles_count)};

    for (unsigned i = 0; i < handles_count; ++i) {
        j6_handle_t h = handles[i];
        if (h == j6_handle_invalid)
            continue;

        kobject *obj = get_handle<kobject>(h);
        if (!obj)
            return j6_err_invalid_arg;

        j6_signal_t current = obj->signals();
        if ((current & mask) != 0) {
            *signals = current;
            *handle = h;
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

    *handle = j6_handle_invalid;
    *signals = th.get_wait_data();
    j6_koid_t koid = th.get_wait_object();
    for (unsigned i = 0; i < handles_count; ++i) {
        if (koid == objects[i]->koid())
            *handle = handles[i];
        else
            objects[i]->remove_blocked_thread(&th);
    }

    kassert(*handle != j6_handle_invalid,
        "Somehow woke on a handle that was not waited on");
    return j6_status_ok;
}

j6_status_t
kobject_signal(j6_handle_t handle, j6_signal_t signals)
{
    if ((signals & j6_signal_user_mask) != signals)
        return j6_err_invalid_arg;

    kobject *obj = get_handle<kobject>(handle);
    if (!obj)
        return j6_err_invalid_arg;

    obj->assert_signal(signals);
    return j6_status_ok;
}

j6_status_t
kobject_close(j6_handle_t handle)
{
    kobject *obj = get_handle<kobject>(handle);
    if (!obj)
        return j6_err_invalid_arg;

    obj->close();
    return j6_status_ok;
}

} // namespace syscalls
