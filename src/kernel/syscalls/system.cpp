#include <j6/errors.h>
#include <j6/types.h>

#include "cpu.h"
#include "device_manager.h"
#include "frame_allocator.h"
#include "logger.h"
#include "memory.h"
#include "objects/event.h"
#include "objects/thread.h"
#include "objects/system.h"
#include "objects/vm_area.h"
#include "syscalls/helpers.h"

extern log::logger &g_logger;

using namespace obj;

namespace syscalls {

using system = class ::system;

j6_status_t
log(uint8_t area, uint8_t severity, const char *message)
{
    const char *name = process::current().name();
    thread &th = thread::current();
    log::log(static_cast<logs>(area), static_cast<log::level>(severity),
        "<%02lx:%02lx> %s: %s", th.parent().obj_id(), th.obj_id(), name, message);
    return j6_status_ok;
}

j6_status_t
noop()
{
    thread &th = thread::current();
    log::verbose(logs::syscall, "Thread <%02lx:%02lx> called noop syscall.", th.parent().obj_id(), th.koid());
    return j6_status_ok;
}

[[ noreturn ]] j6_status_t
test_finish(uint32_t exit_code)
{
    // Tell QEMU to exit
    asm ( "out %0, %1" :: "a"(exit_code+1), "Nd"(0xf4) );
    while (1) asm ("hlt");
}

j6_status_t
system_get_log(system *self, uint64_t seen, void *buffer, size_t *buffer_len)
{
    size_t orig_size = *buffer_len;
    *buffer_len = g_logger.get_entry(seen, buffer, *buffer_len);
    return (*buffer_len > orig_size) ? j6_err_insufficient : j6_status_ok;
}

j6_status_t
system_bind_irq(system *self, event *dest, unsigned irq, unsigned signal)
{
    if (device_manager::get().bind_irq(irq, dest, signal))
        return j6_status_ok;

    return j6_err_invalid_arg;
}

j6_status_t
system_map_phys(system *self, j6_handle_t * area, uintptr_t phys, size_t size, uint32_t flags)
{
    // TODO: check to see if frames are already used? How would that collide with
    // the bootloader's allocated pages already being marked used?
    util::bitset32 f = flags & vm_driver_mask;
    if (!f.get(vm_flags::mmio))
        frame_allocator::get().used(phys, mem::page_count(size));

    construct_handle<vm_area_fixed>(area, phys, size, f);
    return j6_status_ok;
}

j6_status_t
system_request_iopl(system *self, unsigned iopl)
{
    if (iopl != 0 && iopl != 3)
        return j6_err_invalid_arg;

    constexpr uint64_t mask = 3 << 12;
    cpu_data &cpu = current_cpu();
    cpu.rflags3 = (cpu.rflags3 & ~mask) | ((iopl << 12) & mask);
    return j6_status_ok;
}

} // namespace syscalls
