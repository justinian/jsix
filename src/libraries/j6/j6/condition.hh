#pragma once
/// \file condition.hh
/// High level condition interface based on futexes

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stddef.h>
#include <stdint.h>

namespace j6 {

class condition
{
public:
    condition() : m_state {0} {}

    void wait();
    void wake();

private:
    uint32_t m_state;
};

} // namespace j6

#endif // __j6kernel
