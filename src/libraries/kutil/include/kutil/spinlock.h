/// \file spinlock.h
/// Spinlock types and related defintions

#pragma once

namespace kutil {

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
	};

	void acquire(waiter *w);
	void release(waiter *w);

private:
	waiter *m_lock;
};

/// Scoped lock that owns a spinlock::waiter
class scoped_lock
{
public:
	inline scoped_lock(spinlock &lock) : m_lock(lock) {
		m_lock.acquire(&m_waiter);
	}

	inline ~scoped_lock() {
		m_lock.release(&m_waiter);
	}

private:
	spinlock &m_lock;
	spinlock::waiter m_waiter;
};

} // namespace kutil
