// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <arch/memory.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/ring_buffer.hh>
#include <j6/syscalls.h>
#include <j6/types.h>
#include <util/util.h>

namespace j6 {

API
ring_buffer::ring_buffer(size_t pages) :
    m_bits {util::log2(pages) + arch::frame_bits},
    m_write {0},
    m_read {0},
    m_vma {j6_handle_invalid},
    m_data {nullptr}
{
    // Must be a power of 2
    if (!util::is_pow2(pages))
        return;

    uintptr_t buffer_addr = 0;
    size_t size = 1<<m_bits;
    j6_status_t result = j6_vma_create_map(&m_vma, size, &buffer_addr, j6_vm_flag_ring|j6_vm_flag_write);
    if (result != j6_status_ok)
        return;

    m_data = reinterpret_cast<char*>(buffer_addr);
}

} // namespace j6

#endif // __j6kernel
