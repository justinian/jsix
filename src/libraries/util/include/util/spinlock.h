/// \file spinlock.h
/// Spinlock types and related defintions

#pragma once

namespace util {

/// An MCS based spinlock
class spinlock
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
            const char *where = __builtin_FUNCTION()
            ) : m_lock(lock), m_waiter {false, nullptr, where} {
        m_lock.acquire(&m_waiter);
    }

    inline ~scoped_lock() {
        m_lock.release(&m_waiter);
    }

private:
    spinlock &m_lock;
    spinlock::waiter m_waiter;
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
        m_locked = m_lock.try_acquire(&m_waiter);
    }

    inline ~scoped_trylock() {
        if (m_locked)
            m_lock.release(&m_waiter);
    }

    inline bool locked() const { return m_locked; }

private:
    bool m_locked;
    spinlock &m_lock;
    spinlock::waiter m_waiter;
};

} // namespace util
