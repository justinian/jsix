// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/memutils.h>
#include <j6/syscalls.h>
#include <j6/thread.hh>

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
thread::start(void *user)
{
    if (m_status != j6_status_ok)
        return m_status;

    if (m_thread != j6_handle_invalid)
        return j6_err_invalid_arg;

    uint64_t arg0 = reinterpret_cast<uint64_t>(this);
    uint64_t arg1 = reinterpret_cast<uint64_t>(user);

    m_status = j6_thread_create(&m_thread, 0,
        m_stack_top, reinterpret_cast<uintptr_t>(init_proc),
        arg0, arg1);

    return m_status;
}

void
thread::join()
{
    j6_thread_join(m_thread);
}

void
thread::init_proc(thread *t, void *user)
{
    t->m_proc(user);
    j6_thread_exit();
}

} // namespace j6

#endif // __j6kernel
