#include "kutil/spinlock.h"

namespace kutil {

static constexpr int memorder = __ATOMIC_SEQ_CST;

spinlock::spinlock() : m_lock {nullptr} {}
spinlock::~spinlock() {}

void
spinlock::acquire(waiter *w)
{
	w->next = nullptr;
	w->locked = true;

	// Point the lock at this waiter
	waiter *prev = __atomic_exchange_n(&m_lock, w, memorder);
	if (prev) {
		// If there was a previous waiter, wait for them to
		// unblock us
		prev->next = w;
		while (w->locked) {
			asm ("pause");
		}
	} else {
		w->locked = false;
	}
}

void
spinlock::release(waiter *w)
{
	if (!w->next) {
		// If we're still the last waiter, we're done
		if(__atomic_compare_exchange_n(&m_lock, &w, nullptr, false, memorder, memorder))
			return;
	}

	// Wait for the subseqent waiter to tell us who they are
	while (!w->next) {
		asm ("pause");
	}

	// Unblock the subseqent waiter
	w->next->locked = false;
}


} // namespace kutil
