/// \file spinlock.h
/// Spinlock types and related defintions
#pragma once

#include <stdint.h>
#include <util/api.h>

#ifdef __j6kernel
extern "C" uint32_t __current_thread_id();
#endif

namespace util {

/// An MCS based spinlock
class API spinlock
{
public:
    spinlock();
    ~spinlock();

    /// A node in the wait queue.
    struct waiter
    {
        bool blocked;
        waiter *next;
        char const *where;
        uint32_t thread;
    };

    bool try_acquire(waiter *w);
    void acquire(waiter *w);
    void release(waiter *w);

private:
    waiter *m_lock;
};

/// Scoped lock that owns a spinlock::waiter
class scoped_lock
{
public:
    inline scoped_lock(
            spinlock &lock,
            const char *where = __builtin_FUNCTION()) :
        m_lock(lock),
        m_waiter {false, nullptr, where, get_thread()},
        m_is_locked {false}
    {
        m_lock.acquire(&m_waiter);
        m_is_locked = true;
    }

    inline ~scoped_lock() { release(); }

    /// Re-acquire the previously-held lock
    inline void reacquire() {
        if (!m_is_locked) {
            m_lock.acquire(&m_waiter);
            m_is_locked = true;
        }
    }

    /// Release the held lock
    inline void release() {
        if (m_is_locked) {
            m_lock.release(&m_waiter);
            m_is_locked = false;
        }
    }

    inline uint32_t get_thread() const {
#ifdef __j6kernel
        return __current_thread_id();
#else
        return -1u;
#endif
    }

private:
    spinlock &m_lock;
    spinlock::waiter m_waiter;
    bool m_is_locked;
};

/// Scoped lock that owns a spinlock::waiter and calls
/// spinlock::try_acquire
class scoped_trylock
{
public:
    inline scoped_trylock(
            spinlock &lock,
            const char *where = __builtin_FUNCTION()
            ) : m_lock(lock), m_waiter {false, nullptr, where} {
        m_is_locked = m_lock.try_acquire(&m_waiter);
    }

    inline ~scoped_trylock() {
        if (m_is_locked)
            m_lock.release(&m_waiter);
    }

    inline bool locked() const { return m_is_locked; }

private:
    bool m_is_locked;
    spinlock &m_lock;
    spinlock::waiter m_waiter;
};

} // namespace util
