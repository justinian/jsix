#include <j6/errors.h>
#include <j6/signals.h>
#include <j6/types.h>

#include "assert.h"
#include "objects/kobject.h"
#include "objects/thread.h"

namespace obj {

static j6_koid_t next_koids [static_cast<size_t>(kobject::type::max)] = { 0 };

kobject::kobject(type t, j6_signal_t signals) :
    m_koid(koid_generate(t)),
    m_signals(signals),
    m_handle_count(0)
{}

kobject::~kobject()
{
    for (auto *t : m_blocked_threads)
        t->wake_on_result(this, j6_status_destroyed);
}

j6_koid_t
kobject::koid_generate(type t)
{
    kassert(t < type::max, "Object type out of bounds");
    uint64_t type_int = static_cast<uint64_t>(t);
    uint64_t id = __atomic_fetch_add(&next_koids[type_int], 1, __ATOMIC_SEQ_CST);
    return (type_int << 48) | id;
}

kobject::type
kobject::koid_type(j6_koid_t koid)
{
    return static_cast<type>((koid >> 48) & 0xffffull);
}

j6_signal_t
kobject::assert_signal(j6_signal_t s)
{
    j6_signal_t old =
        __atomic_fetch_or(&m_signals, s, __ATOMIC_SEQ_CST);
    notify_signal_observers();
    return old;
}

j6_signal_t
kobject::deassert_signal(j6_signal_t s)
{
    return __atomic_fetch_and(&m_signals, ~s, __ATOMIC_SEQ_CST);
}

void
kobject::compact_blocked_threads()
{
    // Clean up what we can of the list
    while (!m_blocked_threads.empty() && !m_blocked_threads.first())
        m_blocked_threads.pop_front();
    while (!m_blocked_threads.empty() && !m_blocked_threads.last())
        m_blocked_threads.pop_back();
}

void
kobject::notify_signal_observers()
{
    for (auto &entry : m_blocked_threads) {
        if (entry == nullptr) continue;
        if (entry->wake_on_signals(this, m_signals))
            entry = nullptr;
    }
    compact_blocked_threads();
}

void
kobject::notify_object_observers(size_t count)
{
    if (!count) return;

    for (auto &entry : m_blocked_threads) {
        if (entry == nullptr) continue;
        if (entry->wake_on_object(this)) {
            entry = nullptr;
            if (--count) break;
        }
    }
    compact_blocked_threads();
}

void
kobject::remove_blocked_thread(thread *t)
{
    // Can't really remove from a deque, so just
    // null out removed entries.
    for (auto &entry : m_blocked_threads) {
        if (entry == t) {
            entry = nullptr;
            break;
        }
    }
    compact_blocked_threads();
}

void
kobject::close()
{
    assert_signal(j6_signal_closed);
}

void
kobject::on_no_handles()
{
    assert_signal(j6_signal_no_handles);
}

} // namespace obj
