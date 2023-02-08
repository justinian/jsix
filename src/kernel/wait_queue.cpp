#include "objects/thread.h"
#include "wait_queue.h"

wait_queue::~wait_queue() { clear(); }

void
wait_queue::add_thread(obj::thread *t)
{
    util::scoped_lock lock {m_lock};
    t->handle_retain();
    m_threads.push_back(t);
}

void
wait_queue::wait()
{
    obj::thread &current = obj::thread::current();
    add_thread(&current);
    current.block();
}

void
wait_queue::pop_exited()
{
    while (!m_threads.empty()) {
        obj::thread *t = m_threads.first();
        if (!t->exited() && !t->ready())
            break;

        m_threads.pop_front();
        t->handle_release();
    }
}

obj::thread *
wait_queue::get_next_unlocked()
{
    pop_exited();
    if (m_threads.empty())
        return nullptr;
    return m_threads.first();
}

obj::thread *
wait_queue::pop_next_unlocked()
{
    pop_exited();
    if (m_threads.empty())
        return nullptr;
    return m_threads.pop_front();
}

void
wait_queue::clear(uint64_t value)
{
    util::scoped_lock lock {m_lock};
    for (auto *t : m_threads) {
        if (!t->exited()) t->wake(value);
        t->handle_release();
    }
}
