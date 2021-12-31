#include "cpu.h"
#include "display.h"
#include "kutil/printf.h"
#include "serial.h"
#include "symbol_table.h"

namespace panic {

void
print_header(
        serial_port &out,
        const char *message,
        const char *function,
        const char *file,
        uint64_t line)
{
    out.write("\n\n\e[5;31m PANIC:\e[0;1;31m ");
    if (message) {
        out.write(message);
        out.write("\n ");
    }
    out.write(function);
    out.write("  ");
    out.write(file);
    out.write(":");

    char linestr[6];
    snprintf(linestr, sizeof(linestr), "%d", line);
    out.write(linestr);

    out.write("\n \e[0;31m===================================================================================\n");
}

void
print_callstack(serial_port &out, symbol_table &syms, frame const *fp)
{
    char message[512];
    unsigned count = 0;

    while (fp && fp->return_addr) {
        char const *name = syms.find_symbol(fp->return_addr);
        if (!name)
            name = "<unknown>";

        snprintf(message, sizeof(message),
            " \e[0;33mframe %2d: <0x%016lx> \e[1;33m%s\n",
            count++, fp->return_addr, name);

        out.write(message);
        fp = fp->prev;
    }
}

static void
print_reg(serial_port &out, const char *name, uint64_t val, const char *color)
{
    char message[512];

    snprintf(message, sizeof(message), " \e[0;%sm%-3s %016llx\e[0m", color, name, val);
    out.write(message);
}

void
print_cpu_state(serial_port &out, const cpu_state &regs)
{
    out.write("\e[0m\n");

    // Row 1
    print_reg(out, "rsp", regs.rsp, "1;34");
    print_reg(out, "rax", regs.rax, "0;37");
    print_reg(out, "r8",  regs.r8,  "0;37");
    print_reg(out, "r9",  regs.r9,  "0;37");
    out.write("\n");

    // Row 2
    print_reg(out, "rbp", regs.rbp, "1;34");
    print_reg(out, "rbx", regs.rbx, "0;37");
    print_reg(out, "r10", regs.r10, "0;37");
    print_reg(out, "r11", regs.r11, "0;37");
    out.write("\n");

    // Row 3
    print_reg(out, "rdi", regs.rdi, "1;34");
    print_reg(out, "rcx", regs.rcx, "0;37");
    print_reg(out, "r12", regs.r12, "0;37");
    print_reg(out, "r13", regs.r13, "0;37");
    out.write("\n");

    // Row 4
    print_reg(out, "rsi", regs.rdi, "1;34");
    print_reg(out, "rdx", regs.rcx, "0;37");
    print_reg(out, "r14", regs.r12, "0;37");
    print_reg(out, "r15", regs.r13, "0;37");
    out.write("\n");

    // Row 4
    print_reg(out, "rip", regs.rip, "1;35");
    print_reg(out, "ss",  regs.ss,  "1;33");
    print_reg(out, "cs",  regs.cs,  "1;33");
    print_reg(out, "flg", regs.rflags, "0;37");
    out.write("\e[0m\n");
}

} // namespace panic
