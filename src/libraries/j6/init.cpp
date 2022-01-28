// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stdint.h>
#include <j6/errors.h>
#include <j6/init.h>
#include <j6/syscalls.h>
#include <j6/types.h>

j6_handle_t __handle_sys = j6_handle_invalid;
j6_handle_t __handle_self = j6_handle_invalid;

namespace {
    constexpr size_t __static_arr_size = 4;
    j6_handle_t __handle_array[__static_arr_size];

    static j6_status_t
    __load_handles()
    {
        size_t count = __static_arr_size;
        j6_handle_t *handles = __handle_array;
        j6_status_t s = j6_handle_list(handles, &count);

        if (s != j6_err_insufficient && s != j6_status_ok)
            return s;

        if (count > __static_arr_size)
            count = __static_arr_size;

        for (size_t i = 0; i < count; ++i) {
            uint8_t type = (handles[i] >> 56);
            if (type == j6_object_type_system && __handle_sys == j6_handle_invalid)
                __handle_sys = handles[i];
            else if (type == j6_object_type_process && __handle_self == j6_handle_invalid)
                __handle_self = handles[i];
        }

        return s;
    }
} // namespace

extern "C" void
_init_libj6(uint64_t *rsp)
{
    __load_handles();
}

#endif // __j6kernel
