#include "j6/errors.h"
#include "j6/types.h"

#include "device_manager.h"
#include "frame_allocator.h"
#include "log.h"
#include "objects/endpoint.h"
#include "objects/thread.h"
#include "objects/system.h"
#include "objects/vm_area.h"
#include "syscalls/helpers.h"

extern log::logger &g_logger;

namespace syscalls {

j6_status_t
system_log(const char *message)
{
    if (message == nullptr)
        return j6_err_invalid_arg;

    thread &th = thread::current();
    log::info(logs::syscall, "Message[%llx]: %s", th.koid(), message);
    return j6_status_ok;
}

j6_status_t
system_noop()
{
    thread &th = thread::current();
    log::debug(logs::syscall, "Thread %llx called noop syscall.", th.koid());
    return j6_status_ok;
}

j6_status_t
system_get_log(j6_handle_t sys, void *buffer, size_t *size)
{
    if (!size || (*size && !buffer))
        return j6_err_invalid_arg;

    size_t orig_size = *size;
    *size = g_logger.get_entry(buffer, *size);
    if (!g_logger.has_log())
        system::get().deassert_signal(j6_signal_system_has_log);

    return (*size > orig_size) ? j6_err_insufficient : j6_status_ok;
}

j6_status_t
system_bind_irq(j6_handle_t sys, j6_handle_t endp, unsigned irq)
{
    // TODO: check capabilities on sys handle
    endpoint *e = get_handle<endpoint>(endp);
    if (!e) return j6_err_invalid_arg;

    if (device_manager::get().bind_irq(irq, e))
        return j6_status_ok;

    return j6_err_invalid_arg;
}

j6_status_t
system_map_phys(j6_handle_t sys, j6_handle_t *vma_handle, uintptr_t phys_addr, size_t size, uint32_t flags)
{
    // TODO: check capabilities on sys handle
    if (!vma_handle) return j6_err_invalid_arg;

    // TODO: check to see if frames are already used? How would that collide with
    // the bootloader's allocated pages already being marked used?
    if (!(flags & vm_flags::mmio))
        frame_allocator::get().used(phys_addr, memory::page_count(size));

    vm_flags vmf = (static_cast<vm_flags>(flags) & vm_flags::driver_mask);
    construct_handle<vm_area_fixed>(vma_handle, phys_addr, size, vmf);

    return j6_status_ok;
}

} // namespace syscalls
