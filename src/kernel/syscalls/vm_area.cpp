#include "j6/errors.h"
#include "j6/signals.h"
#include "j6/types.h"

#include "log.h"
#include "objects/process.h"
#include "objects/vm_area.h"
#include "syscalls/helpers.h"
#include "vm_space.h"

namespace syscalls {

j6_status_t
vma_create(j6_handle_t *handle, size_t size, uint32_t flags)
{
    vm_flags f = vm_flags::user_mask & flags;
    construct_handle<vm_area_open>(handle, size, f);
    return j6_status_ok;
}

j6_status_t
vma_create_map(j6_handle_t *handle, size_t size, uintptr_t base, uint32_t flags)
{
    vm_flags f = vm_flags::user_mask & flags;
    vm_area *a = construct_handle<vm_area_open>(handle, size, f);
    process::current().space().add(base, a);
    return j6_status_ok;
}

j6_status_t
vma_map(j6_handle_t handle, j6_handle_t proc, uintptr_t base)
{
    vm_area *a = get_handle<vm_area>(handle);
    if (!a) return j6_err_invalid_arg;

    process *p = get_handle<process>(proc);
    if (!p) return j6_err_invalid_arg;

    p->space().add(base, a);
    return j6_status_ok;
}

j6_status_t
vma_unmap(j6_handle_t handle, j6_handle_t proc)
{
    vm_area *a = get_handle<vm_area>(handle);
    if (!a) return j6_err_invalid_arg;

    process *p = get_handle<process>(proc);
    if (!p) return j6_err_invalid_arg;

    p->space().remove(a);
    return j6_status_ok;
}

j6_status_t
vma_resize(j6_handle_t handle, size_t *size)
{
    if (!size)
        return j6_err_invalid_arg;

    vm_area *a = get_handle<vm_area>(handle);
    if (!a) return j6_err_invalid_arg;

    *size = a->resize(*size);
    return j6_status_ok;
}



} // namespace syscalls
