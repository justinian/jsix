#include <util/hash.h>

#include "capabilities.h"
#include "memory.h"

static constexpr size_t one_page_worth = mem::frame_size / sizeof(capability);

inline j6_handle_t make_handle(size_t &counter) {
    //return util::hash(__atomic_fetch_add(&counter, 1, __ATOMIC_RELAXED));
    return __atomic_add_fetch(&counter, 1, __ATOMIC_RELAXED);
}

cap_table::cap_table(uintptr_t start) :
    m_caps(reinterpret_cast<capability*>(start), one_page_worth),
    m_count {0}
{}

j6_handle_t
cap_table::create(obj::kobject *target, j6_cap_t caps)
{
    j6_handle_t new_handle = make_handle(m_count);

    m_caps.insert({
        new_handle,
        j6_handle_invalid,
        0, // starts with a count of 0 holders
        caps,
        target->get_type(),
        target,
        });

    return new_handle;
}

j6_handle_t
cap_table::derive(j6_handle_t base, j6_cap_t caps)
{
    capability *existing = m_caps.find(base);
    if (!existing)
        return j6_handle_invalid;

    caps &= existing->caps;

    j6_handle_t new_handle = make_handle(m_count);

    m_caps.insert({
        new_handle,
        base,
        0, // starts with a count of 0 holders
        caps,
        existing->type,
        existing->object,
        });

    return new_handle;
}

capability *
cap_table::find(j6_handle_t id)
{
    return m_caps.find(id);
}
