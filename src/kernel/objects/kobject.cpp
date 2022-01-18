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

void
kobject::assert_signal(j6_signal_t s)
{
    m_signals |= s;
    notify_signal_observers();
}

void
kobject::deassert_signal(j6_signal_t s)
{
    m_signals &= ~s;
}

void
kobject::notify_signal_observers()
{
    size_t i = 0;
    while (i < m_blocked_threads.count()) {
        thread *t = m_blocked_threads[i];

        if (t->wake_on_signals(this, m_signals)) {
            m_blocked_threads.remove_swap_at(i);
        } else {
            ++i;
        }
    }
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
