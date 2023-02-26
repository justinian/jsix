#pragma once
/// \file mutex.hh
/// High level mutex interface

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stdint.h>

namespace j6 {

class mutex
{
public:
    mutex() : m_state(0) {}

    void lock();
    void unlock();

private:
    uint32_t m_state;
};

class scoped_lock
{
public:
    inline scoped_lock(mutex &m) : m_mutex {m} {
        m_mutex.lock();
    }

    inline ~scoped_lock() {
        m_mutex.unlock();
    }

private:
    mutex &m_mutex;
};

} // namespace j6

#endif // __j6kernel
