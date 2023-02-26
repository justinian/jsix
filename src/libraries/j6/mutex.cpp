// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <j6/mutex.hh>
#include <j6/syscalls.h>

namespace j6 {

void
mutex::lock()
{
    uint32_t lock = 0;
    if ((lock = __sync_val_compare_and_swap(&m_state, 0, 1)) != 0) {
        if (lock != 2)
            lock = __atomic_exchange_n(&m_state, 2, __ATOMIC_ACQ_REL);
        while (lock) {
            j6_futex_wait(&m_state, 2, 0);
            lock = __atomic_exchange_n(&m_state, 2, __ATOMIC_ACQ_REL);
        }
    }
}

void
mutex::unlock()
{
    if (__atomic_fetch_sub(&m_state, 1, __ATOMIC_ACQ_REL) != 1) {
        __atomic_store_n(&m_state, 0, __ATOMIC_RELEASE);
        j6_futex_wake(&m_state, 1);
    }
}


} // namespace j6

#endif // __j6kernel
