#pragma once
/// \file syscalls/helpers.h
/// Utility functions for use in syscall handler implementations

#include <j6/types.h>

#include "objects/kobject.h"
#include "objects/process.h"

namespace syscalls {

template <typename T, typename... Args>
T * construct_handle(j6_handle_t *id, Args... args)
{
    obj::process &p = obj::process::current();
    T *o = new T {args...};
    *id = p.add_handle(o, T::creation_caps);
    return o;
}

template <typename T>
obj::handle * get_handle(j6_handle_t id)
{
    obj::process &p = obj::process::current();
    obj::handle *h = p.lookup_handle(id);
    if (!h || h->type() != T::type)
        return nullptr;
    return h;
}

template <>
inline obj::handle * get_handle<obj::kobject>(j6_handle_t id)
{
    obj::process &p = obj::process::current();
    return p.lookup_handle(id);
}

template <typename T>
T * remove_handle(j6_handle_t id)
{
    obj::handle *h = get_handle<T>(id);
    T *o = nullptr;

    if (h) {
        o = h->object;
        obj::process &p = obj::process::current();
        p.remove_handle(id);
    }
    return o;
}

}
