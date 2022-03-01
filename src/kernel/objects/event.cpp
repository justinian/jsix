#include "objects/event.h"
#include "objects/thread.h"

namespace obj {

event::event() :
    kobject {type::event},
    m_signals {0}
{}

event::~event() {}

void
event::signal(uint64_t s)
{
    uint64_t after = __atomic_or_fetch(&m_signals, s, __ATOMIC_SEQ_CST);
    if (after) wake_observer();
}

uint64_t
event::wait()
{
    // If events are pending, grab those and return immediately
    uint64_t value = read();
    if (value)
        return value;

    // Wait for event::signal() to wake us with a value
    thread &current = thread::current();
    m_queue.add_thread(&current);
    return current.block();
}

void
event::wake_observer()
{
    util::scoped_lock lock {m_queue.get_lock()};
    thread *t = m_queue.get_next_unlocked();
    if (!t) return;

    uint64_t value = read();
    if (value) {
        m_queue.pop_next_unlocked();
        lock.release();
        t->wake(value);
    }
}

} // namespace obj
