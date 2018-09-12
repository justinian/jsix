#pragma once
#include <atomic>

namespace kutil {


class spinlock
{
public:
	spinlock() : m_lock(false) {}

	inline void enter() { while (!m_lock.exchange(true)); }
	inline void leave() { m_lock.store(false); }

private:
	std::atomic<bool> m_lock;
};

} // namespace kutil
