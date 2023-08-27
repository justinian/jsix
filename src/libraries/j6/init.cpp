// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#ifndef __j6kernel

#include <stddef.h>
#include <stdint.h>
#include <j6/errors.h>
#include <j6/init.h>
#include <j6/syscalls.h>
#include <j6/types.h>

namespace {
    constexpr size_t static_arr_count = 8;
    j6_handle_descriptor handle_array[static_arr_count];
    j6_init_args init_args;
} // namespace

j6_handle_t
j6_find_first_handle(j6_object_type obj_type)
{
    size_t count = static_arr_count;
    j6_handle_descriptor *handles = handle_array;
    j6_status_t s = j6_handle_list(handles, &count);

    if (s != j6_err_insufficient && s != j6_status_ok)
        return j6_handle_invalid;

    if (count > static_arr_count)
        count = static_arr_count;

    for (size_t i = 0; i < count; ++i) {
        j6_handle_descriptor &desc = handle_array[i];
        if (desc.type == obj_type) return desc.handle;
    }

    return j6_handle_invalid;
}

const j6_init_args *
j6_get_init_args()
{
    return &init_args;
}

extern "C" void
__init_libj6(uint64_t arg0, uint64_t arg1)
{
    init_args.args[0] = arg0;
    init_args.args[1] = arg1;
}


#endif // __j6kernel
