#pragma once
/// \file kobject.h
/// Definition of base type for user-interactable kernel objects

#include <j6/errors.h>
#include <j6/types.h>

namespace obj {

class thread;

/// Base type for all user-interactable kernel objects
class kobject
{
public:
    /// Types of kernel objects.
    enum class type : uint8_t
    {
#define OBJECT_TYPE( name, val ) name = val,
#include <j6/tables/object_types.inc>
#undef OBJECT_TYPE

        max
    };

    kobject(type t);
    virtual ~kobject();

    /// Generate a new koid for a given type
    /// \arg t    The object type
    /// \returns  A new unique koid
    static j6_koid_t koid_generate(type t);

    /// Get the kobject type from a given koid
    /// \arg koid  An existing koid
    /// \returns   The object type for the koid
    static type koid_type(j6_koid_t koid);

    /// Get this object's type
    inline type get_type() const { return koid_type(m_koid); }

    /// Get this object's koid
    inline j6_koid_t koid() const { return m_koid; }

    /// Increment the handle refcount
    inline void handle_retain() { ++m_handle_count; }

    /// Decrement the handle refcount
    inline void handle_release() {
        if (--m_handle_count == 0) on_no_handles();
    }

protected:
    /// Interface for subclasses to handle when all handles are closed.
    /// Default implementation deletes the object.
    virtual void on_no_handles();

    /// Get the current number of handles to this object
    inline uint16_t handle_count() const { return m_handle_count; }

private:
    kobject() = delete;
    kobject(const kobject &other) = delete;
    kobject(const kobject &&other) = delete;

    j6_koid_t m_koid;
    uint16_t m_handle_count;
};

} // namespace obj
