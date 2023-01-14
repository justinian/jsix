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

    util::scoped_lock lock {m_lock};

    m_caps.insert({
        new_handle,
        j6_handle_invalid,
        0, // starts with a count of 0 holders
        caps,
        target->get_type(),
        target,
        });

    if (target)
        target->handle_retain();

    return new_handle;
}

j6_handle_t
cap_table::derive(j6_handle_t base, j6_cap_t caps)
{
    util::scoped_lock lock {m_lock};

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

    if (existing->object)
        existing->object->handle_retain();

    return new_handle;
}

capability *
cap_table::find_without_retain(j6_handle_t id)
{
    return m_caps.find(id);
}

capability *
cap_table::retain(j6_handle_t id)
{
    util::scoped_lock lock {m_lock};

    capability *cap = m_caps.find(id);
    if (!cap)
        return nullptr;

    ++cap->holders;
    return cap;
}

void
cap_table::release(j6_handle_t id)
{
    util::scoped_lock lock {m_lock};

    capability *cap = m_caps.find(id);
    if (!cap)
        return;

    if (--cap->holders == 0) {
        if (cap->object)
            cap->object->handle_release();
        m_caps.erase(id);
    }
}
