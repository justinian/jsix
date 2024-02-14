#pragma once
/// \file thread.hh
/// High level threading interface

// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stddef.h>
#include <stdint.h>
#include <j6/flags.h>
#include <j6/memutils.h>
#include <j6/types.h>
#include <j6/syscalls.h>

namespace j6 {

template <typename Proc>
class thread
{
public:
    static constexpr uintptr_t stack_base_start = 0x7f0'0000'0000;

    /// Constructor. Create a thread and its stack space, but
    /// do not start executing the thread.
    /// \arg p           The function where the thread will begin execution
    /// \arg stack_top   The address where the top of this thread's stack should be mapped
    /// \arg stack_size  Size of the stack, in bytes (default 16MiB)
    thread(Proc p, size_t stack_size = 0x100'0000) :
        m_stack {j6_handle_invalid},
        m_thread {j6_handle_invalid},
        m_proc {p}
    {
        uintptr_t stack_base = stack_base_start;
        m_status = j6_vma_create_map(&m_stack, stack_size, &stack_base, j6_vm_flag_write);
        if (m_status != j6_status_ok)
            return;

        m_stack_top = stack_base + stack_size;

        static constexpr size_t zeros_size = 0x10;
        m_stack_top -= zeros_size; // Sentinel
        memset(reinterpret_cast<void*>(m_stack_top), 0, zeros_size);
    }

    /// Start executing the thread.
    /// \returns   j6_status_ok if the thread was successfully started.
    j6_status_t start()
    {
        if (m_status != j6_status_ok)
            return m_status;

        if (m_thread != j6_handle_invalid)
            return j6_err_invalid_arg;

        uint64_t arg0 = reinterpret_cast<uint64_t>(this);

        m_status = j6_thread_create(&m_thread, 0,
            m_stack_top, reinterpret_cast<uintptr_t>(init_proc),
            arg0, 0);

        return m_status;
    }

    /// Wait for the thread to stop executing.
    void join() { j6_thread_join(m_thread); }

    thread() = delete;
    thread(const thread&) = delete;

private:
    __attribute__ ((force_align_arg_pointer))
    static void init_proc(thread *t)
    {
        t->m_proc();
        j6_thread_exit();
    }

    j6_status_t m_status;
    j6_handle_t m_stack;
    j6_handle_t m_thread;
    uintptr_t m_stack_top;
    Proc m_proc;
};

} // namespace j6

#endif // __j6kernel
