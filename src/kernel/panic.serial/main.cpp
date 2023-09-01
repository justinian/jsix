#include <util/new.h>
#include <util/no_construct.h>

#include "cpu.h"
#include "display.h"
#include "io.h"
#include "serial.h"
#include "symbol_table.h"

struct cpu_state;

bool main_cpu_done = false;
bool asserting_locked = false;
unsigned remaining = 0;

util::no_construct<panicking::serial_port> __com1_storage;
panicking::serial_port &com1 = __com1_storage.value;

util::no_construct<panicking::symbol_table> __syms_storage;
panicking::symbol_table &syms = __syms_storage.value;

static constexpr int order = __ATOMIC_ACQ_REL;

extern "C"
void panic_handler(const cpu_state *regs)
{
    cpu_data &cpu = current_cpu();
    panic_data *panic = cpu.panic;

    // If we're not running on the CPU that panicked, wait
    // for it to finish
    if (!panic) {
        while (!main_cpu_done) asm ("pause");
    } else {
        new (&com1) panicking::serial_port {panicking::COM1};
        new (&syms) panicking::symbol_table {panic->symbol_data};
        remaining = panic->cpus;
    }

    while (__atomic_test_and_set(&asserting_locked, order))
        asm ("pause");

    panicking::frame const *fp = nullptr;
    asm ( "movq (%%rbp), %0" : "=r" (fp) );

    if (panic)
        print_header(com1, panic->message, panic->function,
                panic->file, panic->line);

    print_cpu(com1, cpu);
    print_callstack(com1, syms, fp);
    print_cpu_state(com1, *regs);

    if (panic && panic->user_state)
        print_user_state(com1, *panic->user_state);

    __atomic_clear(&asserting_locked, order);

    // If we're running on the CPU that panicked, tell the
    // others we have finished
    main_cpu_done = true;

    if (__atomic_sub_fetch(&remaining, 1, order) == 0) {
        // No remaining CPUs, if we're running on QEMU,
        // tell it to exit
        constexpr uint32_t exit_code = 255;
        asm ( "outl %%eax, %%dx" :: "a"(exit_code), "d"(0xf4) );
    }

    while (1) asm ("hlt");
}

#define NDEBUG
#include "../cpprt.cpp"
