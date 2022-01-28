#pragma once
/// \file handle.h
/// Definition of kobject handles

#include <j6/types.h>
#include "objects/kobject.h"

namespace obj {

struct handle
{
    // A j6_handle_t is an id in the low 32 bits, caps in bits 32-55, and type in 56-63
    static inline j6_handle_t make_id(j6_handle_t id, j6_cap_t caps, kobject *obj) {
        return (id & 0xffffffffull) |
            static_cast<j6_handle_t>(caps) << 32 |
            static_cast<j6_handle_t>(obj ? obj->get_type() : kobject::type::none) << 56;
    }

    inline handle(j6_handle_t in_id, kobject *in_obj, j6_cap_t caps) :
        id {make_id(in_id, caps, in_obj)}, object {in_obj} {
        if (object) object->handle_retain();
    }

    inline handle(const handle &other) :
        id {other.id}, object {other.object} {
        if (object) object->handle_retain();
    }

    inline handle(handle &&other) :
        id {other.id}, object {other.object} {
        other.id = 0;
        other.object = nullptr;
    }

    inline handle & operator=(const handle &other) {
        if (object) object->handle_release();
        id = other.id; object = other.object;
        if (object) object->handle_retain();
        return *this;
    }

    inline handle & operator=(handle &&other) {
        if (object) object->handle_release();
        id = other.id; object = other.object;
        other.id = 0; other.object = nullptr;
        return *this;
    }

    inline ~handle() {
        if (object) object->handle_release();
    }

    inline j6_cap_t caps() const { return id >> 32; }

    inline bool has_cap(j6_cap_t test) const {
        return (caps() & test) == test;
    }

    inline kobject::type type() const {
        return static_cast<kobject::type>(id >> 56);
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
    kobject *object;
};

} // namespace obj
