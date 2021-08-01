#include "display.h"
#include "serial.h"
#include "symbol_table.h"

struct cpu_state;

extern "C"
void panic_handler(
        const char *message,
        const void *symbol_data,
        const cpu_state *regs)
{
    panic::serial_port com1(panic::COM1);
    panic::symbol_table syms(symbol_data);

    panic::frame const *fp = nullptr;
	asm ( "mov %%rbp, %0" : "=r" (fp) );

    print_header(com1, message);
    print_callstack(com1, syms, fp);
    print_cpu_state(com1, *regs);

    while (1) asm ("hlt");
}
