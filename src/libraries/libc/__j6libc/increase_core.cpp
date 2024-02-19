#include <stddef.h>
#include <stdint.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/syscalls.h>

namespace __j6libc {

static j6_handle_t core_handle = j6_handle_invalid;
static intptr_t core_size = 0;

static uintptr_t core_base = 0x1c0000000;

static const void *error_val = (void*)-1;

void * increase_core(intptr_t i)
{
	if (i == 0)
		return (void*)core_base;

	if (core_size == 0) {
		if (i < 0)
			return (void*)-1;

		j6_status_t result = j6_vma_create_map(&core_handle, i, &core_base, j6_vm_flag_write);
		if (result != j6_status_ok)
			return (void*)-1;

		core_size = i;
		return (void*)core_base;
	}

	size_t new_size = core_size + i;
	j6_status_t result = j6_vma_resize(core_handle, &new_size);
	if (result != j6_status_ok)
		return (void*)-1;

	uintptr_t prev = core_base + core_size;
	core_size += i;

	return (void*)prev;
}

} // namespace __j6libc
