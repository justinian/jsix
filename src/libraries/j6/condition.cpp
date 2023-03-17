// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <j6/condition.hh>
#include <j6/errors.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>

namespace j6 {

void
condition::wait()
{
    j6::syslog("Waiting on condition %lx", this);
    uint32_t v = __atomic_add_fetch(&m_state, 1, __ATOMIC_ACQ_REL);
    j6_status_t s = j6_futex_wait(&m_state, v, 0);
    while (s == j6_status_futex_changed) {
        v = m_state;
        if (v == 0) break;
        s = j6_futex_wait(&m_state, v, 0);
    }
    j6::syslog("Woke on condition %lx", this);
}

void
condition::wake()
{
    uint32_t v = __atomic_exchange_n(&m_state, 0, __ATOMIC_ACQ_REL);
    if (v)
        j6_futex_wake(&m_state, 0);
}


} // namespace j6

#endif // __j6kernel
