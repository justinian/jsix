#pragma once

#include <stdint.h>

#include <util/counted.h>
#include "cpu.h"

namespace panic {

[[ noreturn ]]
void panic(
        const char *message = nullptr,
        const cpu_state *user = nullptr,
        const char *function = __builtin_FUNCTION(),
        const char *file = __builtin_FILE(),
        uint64_t line = __builtin_LINE());

/// Install a panic handler.
/// \arg entrypoint   Virtual address of the panic handler's entrypoint
/// \arg symbol_data  Symbol table data
void install(uintptr_t entrypoint, util::const_buffer symbol_data);

} // namespace panic

#ifndef NDEBUG

__attribute__ ((always_inline))
inline void kassert(
        bool check,
        const char *message = nullptr,
        const cpu_state *user = nullptr,
        const char *function = __builtin_FUNCTION(),
        const char *file = __builtin_FILE(),
        uint64_t line = __builtin_LINE())
{
    if (!check)
        panic::panic(message, user, function, file, line);
}

#define assert(x) kassert((x))

#else // NDEBUG

#define kassert(...) ((void)0)
#define assert(x) ((void)0)

#endif // NDEBUG
