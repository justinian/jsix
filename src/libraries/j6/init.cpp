// The kernel depends on libj6 for some shared code,
// but should not include the user-specific code.
#include "j6/init.h"
#include "j6/types.h"
#ifndef __j6kernel

#include <stddef.h>
#include <stdint.h>
#include <j6/errors.h>
#include <j6/init.h>
#include <j6/syscalls.h>
#include <j6/types.h>

namespace {
    char const * const *envp = nullptr;
    const j6_aux *aux = nullptr;

    const j6_aux * find_aux(uint64_t type) {
        if (!aux) return nullptr;
        for (j6_aux const *p = aux; p->type; ++p)
            if (p->type == type) return p;
        return nullptr;
    }
} // namespace

j6_handle_t
j6_find_init_handle(uint64_t proto)
{
    const j6_aux *aux_handles = find_aux(j6_aux_handles);
    if (!aux_handles)
        return j6_handle_invalid;

    const j6_arg_handles *arg = reinterpret_cast<const j6_arg_handles*>(aux_handles->pointer);
    for (unsigned i = 0; i < arg->nhandles; ++i) {
        const j6_arg_handle_entry &ent = arg->handles[i];
        if (ent.proto == proto)
            return ent.handle;
    }

    return j6_handle_invalid;
}

extern "C" void API
__init_libj6(const uint64_t *stack)
{
    // Walk the stack to get the aux vector
    uint64_t argc = *stack++;
    stack += argc + 1; // Skip argv's and sentinel

    envp = reinterpret_cast<char const * const *>(stack);
    while (*stack++); // Skip envp's and sentinel

    aux = reinterpret_cast<const j6_aux*>(stack);
}

#endif // __j6kernel
