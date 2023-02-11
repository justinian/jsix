#include "assert.h"
#include "idt.h"

namespace panic {

uint32_t *apic_icr = reinterpret_cast<uint32_t*>(0xffffc000fee00300);
void const *symbol_table = nullptr;

void
install(uintptr_t entrypoint, util::const_buffer symbol_data)
{
    IDT::set_nmi_handler(entrypoint);
    symbol_table = symbol_data.pointer;
}

extern uint32_t *apic_icr;
extern void const *symbol_table;

[[ noreturn ]]
void panic(
        const char *message,
        const cpu_state *user,
        const char *function,
        const char *file,
        uint64_t line)
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

        static constexpr uint32_t send_nmi_command =
            (4 <<  8) | // Delivery mode NMI
            (1 << 14) | // assert level high
            (2 << 18);  // destination all

        *apic_icr = send_nmi_command;
    }

    while (1) asm ("hlt");
}

} // namespace panic

extern "C"
void __assert_fail(const char *message, const char *file, unsigned line, const char *function) {
    panic::panic(message, nullptr, function, file, line);
}
