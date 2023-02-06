#pragma once

#include <stdint.h>

#include <util/counted.h>
#include "cpu.h"

namespace panic {

constexpr uint32_t send_nmi_command =
    (4 << 8) |  // Delivery mode NMI
    (1 << 14) | // assert level high
    (2 << 18);  // destination all

extern uint32_t *apic_icr;
extern void const *symbol_table;

[[ noreturn ]]
__attribute__ ((always_inline))
inline void panic(
        const char *message = nullptr,
        const cpu_state *user = nullptr,
        const char *function = __builtin_FUNCTION(),
        const char *file = __builtin_FILE(),
        uint64_t line = __builtin_LINE())
{
    cpu_data &cpu = current_cpu();

    // Grab the global panic block for ourselves
    cpu.panic = __atomic_exchange_n(&g_panic_data_p, nullptr, __ATOMIC_ACQ_REL);

    // If we aren't the first CPU to panic, cpu.panic will be null
    if (cpu.panic) {
        cpu.panic->symbol_data = symbol_table;
        cpu.panic->user_state = user;

        cpu.panic->message = message;
        cpu.panic->function = function;
        cpu.panic->file = file;
        cpu.panic->line = line;

        cpu.panic->cpus = g_num_cpus;

        *apic_icr = send_nmi_command;
    }

    while (1) asm ("hlt");
}

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
