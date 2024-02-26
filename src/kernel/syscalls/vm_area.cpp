#include <j6/errors.h>
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
    util::bitset32 f = flags & vm_user_mask;
    if (f.get(vm_flags::ring))
        construct_handle<vm_area_ring>(self, size, f);
    else
        construct_handle<vm_area_open>(self, size, f);
    return j6_status_ok;
}

j6_status_t
vma_create_map(j6_handle_t *self, size_t size, uintptr_t *base, uint32_t flags)
{
    vm_area *a = nullptr;
    util::bitset32 f = flags & vm_user_mask;
    if (f.get(vm_flags::ring))
        a = construct_handle<vm_area_ring>(self, size, f);
    else
        a = construct_handle<vm_area_open>(self, size, f);

    *base = process::current().space().add(*base, a, f);
    return *base ? j6_status_ok : j6_err_collision;
}

j6_status_t
vma_map(vm_area *self, process *proc, uintptr_t *base, uint32_t flags)
{
    vm_space &space = proc ? proc->space() : process::current().space();
    util::bitset32 f = flags & vm_user_mask;
    *base = space.add(*base, self, f);
    return *base ? j6_status_ok : j6_err_collision;
}

j6_status_t
vma_unmap(vm_area *self, process *proc)
{
    vm_space &space = proc ? proc->space() : process::current().space();
    space.remove(self);
    return j6_status_ok;
}

j6_status_t
vma_resize(vm_area *self, size_t *size)
{
    if (!*size) {
        *size = self->size();
    } else {
        *size = self->resize(*size);
    }
    return j6_status_ok;
}



} // namespace syscalls
