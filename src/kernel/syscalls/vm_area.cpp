#include <j6/errors.h>
#include <j6/signals.h>
#include <j6/types.h>

#include "logger.h"
#include "objects/process.h"
#include "objects/vm_area.h"
#include "syscalls/helpers.h"
#include "vm_space.h"

using namespace obj;

namespace syscalls {

j6_status_t
vma_create(j6_handle_t *self, size_t size, uint32_t flags)
{
    vm_flags f = vm_flags::user_mask & flags;
    construct_handle<vm_area_open>(self, size, f);
    return j6_status_ok;
}

j6_status_t
vma_create_map(j6_handle_t *self, size_t size, uintptr_t base, uint32_t flags)
{
    vm_flags f = vm_flags::user_mask & flags;
    vm_area *a = construct_handle<vm_area_open>(self, size, f);
    process::current().space().add(base, a);
    return j6_status_ok;
}

j6_status_t
vma_map(vm_area *self, process *proc, uintptr_t base)
{
    proc->space().add(base, self);
    return j6_status_ok;
}

j6_status_t
vma_unmap(vm_area *self, process *proc)
{
    proc->space().remove(self);
    return j6_status_ok;
}

j6_status_t
vma_resize(vm_area *self, size_t *size)
{
    *size = self->resize(*size);
    return j6_status_ok;
}



} // namespace syscalls
