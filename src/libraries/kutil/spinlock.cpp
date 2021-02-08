#include "kutil/spinlock.h"

namespace kutil {
namespace spinlock {

static constexpr int memorder = __ATOMIC_SEQ_CST;

void
aquire(node * &lock, node *waiter)
{
	waiter->next = nullptr;
	waiter->locked = true;

	// Point the lock at this waiter
	node *prev = __atomic_exchange_n(&lock, waiter, memorder);
	if (prev) {
		// If there was a previous waiter, wait for them to
		// unblock us
		prev->next = waiter;
		while (waiter->locked) {
			asm ("pause");
		}
	} else {
		waiter->locked = false;
	}
}

void
release(node * &lock, node *waiter)
{
	if (!waiter->next) {
		// If we're still the last waiter, we're done
		if(__atomic_compare_exchange_n(&lock, &waiter, nullptr, false, memorder, memorder))
			return;
	}

	// Wait for the subseqent waiter to tell us who they are
	while (!waiter->next) {
		asm ("pause");
	}

	// Unblock the subseqent waiter
	waiter->next->locked = false;
}


} // namespace spinlock
} // namespace kutil
