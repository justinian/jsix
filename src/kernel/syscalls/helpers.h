#pragma once
/// \file syscalls/helpers.h
/// Utility functions for use in syscall handler implementations

#include <j6/types.h>

#include "objects/kobject.h"
#include "objects/process.h"

namespace syscalls {

template <typename T, typename... Args>
T * construct_handle(j6_handle_t *handle, Args... args)
{
    process &p = process::current();
    T *o = new T {args...};
    *handle = p.add_handle(o);
    return o;
}

template <typename T>
T * get_handle(j6_handle_t handle)
{
    process &p = process::current();
    kobject *o = p.lookup_handle(handle);
    if (!o || o->get_type() != T::type)
        return nullptr;
    return static_cast<T*>(o);
}

template <>
inline kobject * get_handle<kobject>(j6_handle_t handle)
{
    process &p = process::current();
    return p.lookup_handle(handle);
}

template <typename T>
T * remove_handle(j6_handle_t handle)
{
    T *o = get_handle<T>(handle);
    if (o) {
        process &p = process::current();
        p.remove_handle(handle);
    }
    return o;
}

}
