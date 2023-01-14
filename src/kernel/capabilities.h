#pragma once
/// \file capabilities.h
/// Capability table definitions

#include <j6/types.h>
#include <util/node_map.h>
#include <util/spinlock.h>

#include "objects/kobject.h"

struct capability
{
    j6_handle_t id;
    j6_handle_t parent;

    uint32_t holders;
    j6_cap_t caps;
    obj::kobject::type type;
    // 1 byte alignment padding

    obj::kobject *object;
};

class cap_table
{
public:
    /// Constructor. Takes the start of the memory region to contain the cap table.
    cap_table(uintptr_t start);

    /// Create a new capability for the given object.
    j6_handle_t create(obj::kobject *target, j6_cap_t caps);

    /// Create a new capability from an existing one.
    j6_handle_t derive(j6_handle_t id, j6_cap_t caps);

    /// Get the capability referenced by a handle.
    capability * retain(j6_handle_t id);

    /// Release a retained capability when it is no longer being used.
    void release(j6_handle_t id);

    /// Get the capability referenced by a handle, without increasing
    /// its refcount. WARNING: You probably want retain(), using a capability
    /// (or the object it points to) without retaining it is dangerous.
    capability * find_without_retain(j6_handle_t id);

private:
    using map_type = util::inplace_map<j6_handle_t, capability, j6_handle_invalid>;
    map_type m_caps;

    util::spinlock m_lock;
    size_t m_count;
};

extern cap_table &g_cap_table;
inline uint64_t & get_map_key(capability &cap) { return cap.id; }
