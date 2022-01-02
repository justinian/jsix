#include "kutil/spinlock.h"

#include "assert.h"
#include "display.h"
#include "serial.h"
#include "symbol_table.h"

struct cpu_state;

extern "C"
void panic_handler(
        const void *symbol_data,
        const char *message,
        const char *function,
        const char *file,
        uint64_t line,
        const cpu_state *regs)
{
    panicking::serial_port com1(panicking::COM1);
    panicking::symbol_table syms(symbol_data);

    panicking::frame const *fp = nullptr;
    asm ( "mov %%rbp, %0" : "=r" (fp) );

    print_header(com1, message, function, file, line);
    print_callstack(com1, syms, fp);
    print_cpu_state(com1, *regs);

    while (1) asm ("hlt");
}
