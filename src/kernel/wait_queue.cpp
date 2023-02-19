#include "objects/thread.h"
#include "wait_queue.h"

wait_queue::~wait_queue() { clear(); }

void
wait_queue::add_thread(obj::thread *t)
{
    kassert(t, "Adding a null thread to the wait queue");
    util::scoped_lock lock {m_lock};
    t->handle_retain();
    m_threads.push_back(t);
}

void
wait_queue::wait()
{
    obj::thread &current = obj::thread::current();

    util::scoped_lock lock {m_lock};
    current.handle_retain();
    m_threads.push_back(&current);

    current.block(lock);
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
wait_queue::pop_next()
{
    pop_exited();
    if (m_threads.empty())
        return nullptr;

    obj::thread *t = m_threads.pop_front();
    kassert(t, "Null thread in wait_queue");
    t->handle_release();
    return t;
}

void
wait_queue::clear(uint64_t value)
{
    obj::thread *t = pop_next();
    while (t) {
        t->wake(value);
        t = pop_next();
    }
}
