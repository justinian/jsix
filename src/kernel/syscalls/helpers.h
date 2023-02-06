#pragma once
/// \file syscalls/helpers.h
/// Utility functions for use in syscall handler implementations

#include <j6/types.h>

#include "capabilities.h"
#include "objects/kobject.h"
#include "objects/process.h"

namespace syscalls {

template <typename T, typename... Args>
T * construct_handle(j6_handle_t *id, Args... args)
{
    T *o = new T {args...};
    *id = g_cap_table.create(o, T::creation_caps);

    obj::process &p = obj::process::current();
    p.add_handle(*id);

    return o;
}

template <typename T>
j6_status_t get_handle(j6_handle_t id, j6_cap_t caps, T *&object, bool optional = false)
{
    if (id == j6_handle_invalid) {
        object = nullptr;
        return optional ? j6_status_ok : j6_err_invalid_arg;
    }

    capability *capdata = g_cap_table.retain(id);
    if (!capdata || capdata->type != T::type)
        return j6_err_invalid_arg;

    obj::process &p = obj::process::current();
    if (!p.has_handle(id) || (capdata->caps & caps) != caps)
        return j6_err_denied;

    object = static_cast<T*>(capdata->object);
    return j6_status_ok;
}

template <typename T>
inline j6_status_t get_handle(j6_handle_t *id, j6_cap_t caps, T *&object, bool optional = false)
{
    return get_handle<T>(*id, caps, object, optional);
}

template <>
inline j6_status_t get_handle<obj::kobject>(j6_handle_t id, j6_cap_t caps, obj::kobject *&object, bool optional)
{
    if (id == j6_handle_invalid) {
        object = nullptr;
        return optional ? j6_status_ok : j6_err_invalid_arg;
    }

    capability *capdata = g_cap_table.retain(id);
    if (!capdata)
        return j6_err_invalid_arg;

    obj::process &p = obj::process::current();
    if (!p.has_handle(id) || (capdata->caps & caps) != caps)
        return j6_err_denied;

    object = capdata->object;
    return j6_status_ok;
}

inline void release_handle(j6_handle_t id) { g_cap_table.release(id); }
inline void release_handle(j6_handle_t *id) { g_cap_table.release(*id); }

} // namespace syscalls
