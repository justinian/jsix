// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stddef.h>
#include <stdint.h>
#include <j6/errors.h>
#include <j6/init.h>
#include <j6/syscalls.h>
#include <j6/types.h>

j6_handle_t __handle_self;

namespace {
    constexpr size_t static_arr_size = 8;
    j6_handle_t handle_array[static_arr_size];
} // namespace

j6_handle_t
j6_find_first_handle(j6_object_type obj_type)
{
    size_t count = static_arr_size;
    j6_handle_t *handles = handle_array;
    j6_status_t s = j6_handle_list(handles, &count);

    if (s != j6_err_insufficient && s != j6_status_ok)
        return j6_handle_invalid;

    if (count > static_arr_size)
        count = static_arr_size;

    for (size_t i = 0; i < count; ++i) {
        uint8_t type = (handles[i] >> 56);
        if (type == obj_type) return handles[i];
    }

    return j6_handle_invalid;
}

extern "C" void
__init_libj6(uint64_t *rsp)
{
    __handle_self = j6_find_first_handle(j6_object_type_process);
}

#endif // __j6kernel
