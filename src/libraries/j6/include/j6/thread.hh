#pragma once
/// \file thread.hh
/// High level threading interface

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stddef.h>
#include <stdint.h>
#include <j6/types.h>

namespace j6 {

class thread
{
public:
    using proc = void (*)(void *);

    /// Constructor. Create a thread and its stack space, but
    /// do not start executing the thread.
    /// \arg p         The function where the thread will begin execution
    /// \arg stack_top The address where the top of this thread's stack should be mapped
    thread(proc p, uintptr_t stack_top);

    /// Start executing the thread.
    /// \arg user  Optional pointer to user data to pass to the thread proc
    /// \returns   j6_status_ok if the thread was successfully started.
    j6_status_t start(void *user = nullptr);

    /// Wait for the thread to stop executing.
    void join();

    thread() = delete;
    thread(const thread&) = delete;

private:
    static void init_proc(thread *t, void *user);

    j6_status_t m_status;
    j6_handle_t m_stack;
    j6_handle_t m_thread;
    uintptr_t m_stack_top;
    proc m_proc;
};

} // namespace j6

#endif // __j6kernel
