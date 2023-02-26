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
    wait_queue() = default;
    wait_queue(wait_queue &&other);

    wait_queue & operator=(wait_queue &&other);

    /// Wake all threads when destructing
    ~wait_queue();

    /// Add the given thread to the queue.
    void add_thread(obj::thread *t);

    /// Block the current thread on the queue.
    void wait();

    /// Pops the next waiting thread off the queue.
    obj::thread * pop_next();

    /// Wake and clear out all threads.
    /// \arg value  The value passed to thread::wake
    void clear(uint64_t value = 0);

    /// Check if the queue is empty
    bool empty() const { return m_threads.empty(); }

private:
    /// Get rid of any exited threads that are next
    /// in the queue. Caller must hold the queue lock.
    void pop_exited();

    util::spinlock m_lock;
    util::deque<obj::thread*, 6> m_threads;
};

