// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <string.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/syscalls.h>
#include <j6/thread.hh>

extern j6_handle_t __handle_self;

namespace j6 {

static constexpr size_t stack_size = 0x10000;
static constexpr size_t zeros_size = 0x10;

thread::thread(thread::proc p, uintptr_t stack_top) :
    m_stack {j6_handle_invalid},
    m_thread {j6_handle_invalid},
    m_stack_top {stack_top},
    m_proc {p}
{
    uintptr_t stack_base = stack_top - stack_size;
    m_status = j6_vma_create_map(&m_stack, stack_size, stack_base, j6_vm_flag_write);
    if (m_status != j6_status_ok)
        return;

    m_stack_top -= zeros_size;
    memset(reinterpret_cast<void*>(m_stack_top), 0, zeros_size);
}

j6_status_t
thread::start()
{
    if (m_status != j6_status_ok)
        return m_status;

    if (m_thread != j6_handle_invalid)
        return j6_err_invalid_arg;

    m_status = j6_thread_create(&m_thread, __handle_self,
        m_stack_top, reinterpret_cast<uintptr_t>(m_proc));

    return m_status;
}

void
thread::join()
{
    j6_thread_join(m_thread);
}

}

#endif // __j6kernel
