#include <stdint.h>
#include <j6/errors.h>
#include <j6/syscalls.h>
//void *sbrk(intptr_t) __attribute__ ((weak));

static j6_handle_t __core_handle = 0;
static intptr_t __core_size = 0;

static const uintptr_t __core_base = 0x1c0000000;

static const void *error_val = (void*)-1;

void *sbrk(intptr_t i)
{
	if (i == 0)
		return (void*)__core_base;

	if (__core_size == 0) {
		if (i < 0)
			return (void*)-1;

		j6_status_t result = j6_vma_create_map(&__core_handle, i, __core_base, 1);
		if (result != j6_status_ok)
			return (void*)-1;

		__core_size = i;
		return (void*)__core_base;
	}

	size_t new_size = __core_size + i;
	j6_status_t result = j6_vma_resize(__core_handle, &new_size);
	if (result != j6_status_ok)
		return (void*)-1;

	uintptr_t prev = __core_base + __core_size;
	__core_size += i;

	return (void*)prev;
}
