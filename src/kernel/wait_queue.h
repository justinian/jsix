#pragma once
/// \file wait_queue.h
/// Class and related defintions for keeping a queue of waiting threads

#include <util/deque.h>
#include <util/spinlock.h>

namespace obj {
    class thread;
}

class wait_queue
{
public:
    /// Wake all threads when destructing
    ~wait_queue();

    /// Add the given thread to the queue. Locks the
    /// queue lock.
    void add_thread(obj::thread *t);

    /// Pops the next waiting thread off the queue.
    /// Locks the queue lock.
    inline obj::thread * pop_next() {
        util::scoped_lock lock {m_lock};
        return pop_next_unlocked();
    }

    /// Get the next waiting thread. Does not lock the
    /// queue lock.
    obj::thread * get_next_unlocked();

    /// Pop the next thread off the queue. Does not
    /// lock the queue lock.
    obj::thread * pop_next_unlocked();

    /// Get the spinlock to lock this queue
    util::spinlock & get_lock() { return m_lock; }

private:
    /// Get rid of any exited threads that are next
    /// in the queue. Caller must hold the queue lock.
    void pop_exited();

    util::spinlock m_lock;
    util::deque<obj::thread*, 8> m_threads;
};

