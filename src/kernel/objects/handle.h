#pragma once
/// \file handle.h
/// Definition of kobject handles

#include <j6/types.h>
#include "objects/kobject.h"

namespace obj {

struct handle
{
    inline handle(j6_handle_t in_id, kobject *in_object, j6_cap_t in_caps) :
        id {in_id}, object {in_object}, caps {in_caps} {
        if (object) object->handle_retain();
    }

    inline handle(const handle &other) :
        id {other.id}, object {other.object}, caps {other.caps} {
        if (object) object->handle_retain();
    }

    inline handle(handle &&other) :
        id {other.id}, object {other.object}, caps {other.caps} {
        other.id = 0;
        other.caps = 0;
        other.object = nullptr;
    }

    inline handle & operator=(const handle &other) {
        if (object) object->handle_release();
        id = other.id; caps = other.caps; object = other.object;
        if (object) object->handle_retain();
        return *this;
    }

    inline handle & operator=(handle &&other) {
        if (object) object->handle_release();
        id = other.id; caps = other.caps; object = other.object;
        other.id = other.caps = 0; other.object = nullptr;
        return *this;
    }

    inline ~handle() {
        if (object) object->handle_release();
    }

    inline bool has_cap(j6_cap_t test) const {
        return (caps & test) == test;
    }

    inline kobject::type type() const {
        return object->get_type();
    }

    inline int compare(const handle &o) {
        return id > o.id ? 1 : id < o.id ? -1 : 0;
    }

    template <typename T>
    inline T * as() {
        if (type() != T::type) return nullptr;
        return reinterpret_cast<T*>(object);
    }

    template <typename T>
    inline const T * as() const {
        if (type() != T::type) return nullptr;
        return reinterpret_cast<const T*>(object);
    }

    template <>
    inline kobject * as<kobject>() { return object; }

    template <>
    inline const kobject * as<kobject>() const { return object; }

    j6_handle_t id;
    j6_cap_t caps;
    kobject *object;
};

} // namespace obj
