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

    /// The koid's most significant bits represent the object's type
    static constexpr unsigned koid_type_bits = 4;
    static constexpr unsigned koid_type_mask = (1 << koid_type_bits) - 1;
    static constexpr unsigned koid_type_shift = (8 * sizeof(j6_koid_t)) - koid_type_bits;

    kobject(type t);
    virtual ~kobject();

    /// Get the kobject type from a given koid
    /// \arg koid  An existing koid
    /// \returns   The object type for the koid
    inline static type koid_type(j6_koid_t koid) {
        return static_cast<type>((koid >> koid_type_shift) & koid_type_mask);
    }

    static const char * type_name(type t);

    /// Get this object's type
    inline type get_type() const { return m_type; }

    /// Get this object's koid
    inline j6_koid_t koid() const {
        return (static_cast<j6_koid_t>(m_type) << koid_type_shift) | m_obj_id;
    }

    /// Get this object's type-relative object id
    inline uint32_t obj_id() const { return m_obj_id; }

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

    uint16_t m_handle_count;
    type m_type;
    uint32_t m_obj_id;
};

} // namespace obj
