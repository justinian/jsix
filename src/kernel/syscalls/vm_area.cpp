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
vma_create(j6_handle_t *handle, size_t size)
{
	construct_handle<vm_area>(handle, size);
	return j6_status_ok;
}

j6_status_t
vma_create_map(j6_handle_t *handle, size_t size, uintptr_t base)
{
	vm_area *a = construct_handle<vm_area>(handle, size);
	a->add_to(&process::current().space(), base);
	return j6_status_ok;
}

j6_status_t
vma_map(j6_handle_t handle, uintptr_t base)
{
	vm_area *a = get_handle<vm_area>(handle);
	if (!a) return j6_err_invalid_arg;

	a->add_to(&process::current().space(), base);
	return j6_status_ok;
}

j6_status_t
vma_unmap(j6_handle_t handle)
{
	vm_area *a = get_handle<vm_area>(handle);
	if (!a) return j6_err_invalid_arg;

	a->remove_from(&process::current().space());
	return j6_status_ok;
}

j6_status_t
vma_close(j6_handle_t handle)
{
	j6_status_t status = vma_unmap(handle);
	if (status != j6_status_ok)
		return status;

	remove_handle<vm_area>(handle);
	return j6_status_ok;
}



} // namespace syscalls
