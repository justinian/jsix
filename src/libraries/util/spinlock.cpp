#include <util/spinlock.h>

namespace util {

static constexpr int memorder = __ATOMIC_SEQ_CST;

spinlock::spinlock() : m_lock {nullptr} {}
spinlock::~spinlock() {}

bool
spinlock::try_acquire(waiter *w)
{
    w->next = nullptr;
    w->blocked = false;

    // Point this lock at the waiter only if it's empty
    waiter *expected = nullptr;
    return __atomic_compare_exchange_n(&m_lock, &expected, w, false, memorder, memorder);
}

void
spinlock::acquire(waiter *w)
{
    w->next = nullptr;
    w->blocked = true;

    // Point the lock at this waiter
    waiter *prev = __atomic_exchange_n(&m_lock, w, memorder);
    if (prev) {
        // If there was a previous waiter, wait for them to
        // unblock us
        prev->next = w;
        while (w->blocked)
            asm ("pause");
    } else {
        w->blocked = false;
    }
}

void
spinlock::release(waiter *w)
{
    // If we're still the last waiter, we're done
    waiter *expected = w;
    if(__atomic_compare_exchange_n(&m_lock, &expected, nullptr, false, memorder, memorder))
        return;

    // Wait for the subseqent waiter to tell us who they are
    while (!w->next)
        asm ("pause");

    // Unblock the subseqent waiter
    w->next->blocked = false;
}


} // namespace util
