#include <util/format.h>

#include "cpu.h"
#include "display.h"
#include "kernel.dir/memory.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "serial.h"
#include "symbol_table.h"

namespace panicking {

template <typename T> inline bool
check_pointer(T p)
{
    static constexpr uint64_t large_flag = (1<<7);
    static constexpr uint64_t pointer_mask = 0x0000fffffffff000;
    static constexpr uintptr_t canon_mask = 0xffff800000000000;

    uintptr_t addr = reinterpret_cast<uintptr_t>(p);

    uintptr_t canon_bits = addr & canon_mask;
    if (canon_bits && canon_bits != canon_mask)
        return false;

    uintptr_t pml4 = 0;
    asm volatile ( "mov %%cr3, %0" : "=r" (pml4) );
    pml4 += mem::linear_offset;

    uint64_t *table = reinterpret_cast<uint64_t*>(pml4);
    unsigned shift = 39;

    while (table) {
        unsigned index = (addr >> shift) & 0x1ffull;
        uint64_t entry = table[index];

        if ((entry & 0x1) == 0)
            return false;

        if ((entry & large_flag) || shift <= 12)
            return true;

        uint64_t next = (entry & pointer_mask) + mem::linear_offset;
        table = reinterpret_cast<uint64_t*>(next);
        shift -= 9;
    }

    return false;
}

const char *clear = "\e[0m\n";

void
print_header(
        serial_port &out,
        const char *message,
        const char *function,
        const char *file,
        uint64_t line)
{
    out.write(clear);
    out.write("\n\e[5;31m PANIC:\e[0;1;31m ");
    if (message) {
        out.write(message);
        out.write("\n ");
    }
    out.write(function);
    out.write("  ");
    out.write(file);
    out.write(":");

    char linestr[6];
    util::format({linestr, sizeof(linestr)}, "%ld", line);
    out.write(linestr);
}

void
print_cpu(serial_port &out, cpu_data &cpu)
{
    uint32_t process = cpu.process ? cpu.process->obj_id() : 0;
    uint32_t thread = cpu.thread ? cpu.thread->obj_id() : 0;
    const char *name = cpu.process ? cpu.process->name() : "<Unknown>";

    out.write("\n \e[0;31m==[ CPU: ");

    char buffer[64];
    size_t len = util::format({buffer, sizeof(buffer)}, "%4d <%02lx:%02lx> ]: %s ",
            cpu.id + 1, process, thread, name);
    out.write(buffer);

    for (size_t i = 0; i < (74-len); ++i) out.write("=");
    out.write("\n");
}

void
print_callstack(serial_port &out, symbol_table &syms, frame const *fp)
{
    char message[512];
    unsigned count = 0;

    while (fp && check_pointer(fp)) {
        const uintptr_t ret = fp->return_addr;
        char const *name = ret ? syms.find_symbol(ret) : "<END>";
        if (!name)
            name = check_pointer(fp->return_addr) ? "<unknown>" : "<unmapped>";

        util::format({message, sizeof(message)},
            " \e[0;33mframe %2d: <0x%016lx> <0x%016lx> \e[1;33m%s\n",
            count++, fp, fp->return_addr, name);

        out.write(message);

        if (!ret) return;
        fp = fp->prev;
    }

    const char *result = fp ? " <inaccessible>" : "";
    util::format({message, sizeof(message)}, " \e[0mfinal     <0x%016lx>%s\n",
            fp, result);
    out.write(message);
}

static void
print_reg(serial_port &out, const char *name, uint64_t val, const char *color)
{
    char message[512];

    util::format({message, sizeof(message)}, " \e[0;%sm%3s %016lx\e[0m", color, name, val);
    out.write(message);
}

void
print_cpu_state(serial_port &out, const cpu_state &regs)
{
    out.write(clear);

    // Row 1
    print_reg(out, "rsp", regs.rsp, "1;34");
    print_reg(out, "rax", regs.rax, "0;37");
    print_reg(out, "r8",  regs.r8,  "0;37");
    print_reg(out, "r12", regs.r12, "0;37");
    out.write("\n");

    // Row 2
    print_reg(out, "rbp", regs.rbp, "1;34");
    print_reg(out, "rbx", regs.rbx, "0;37");
    print_reg(out, "r9",  regs.r9,  "0;37");
    print_reg(out, "r13", regs.r13, "0;37");
    out.write("\n");

    // Row 3
    print_reg(out, "rdi", regs.rdi, "0;37");
    print_reg(out, "rcx", regs.rcx, "0;37");
    print_reg(out, "r10", regs.r10, "0;37");
    print_reg(out, "r14", regs.r14, "0;37");
    out.write("\n");

    // Row 4
    print_reg(out, "rsi", regs.rdi, "0;37");
    print_reg(out, "rdx", regs.rcx, "0;37");
    print_reg(out, "r11", regs.r11, "0;37");
    print_reg(out, "r15", regs.r15, "0;37");
    out.write("\n");

    // Row 4
    print_reg(out, "rip", regs.rip, "1;35");
    print_reg(out, "ss",  regs.ss,  "1;33");
    print_reg(out, "cs",  regs.cs,  "1;33");
    print_reg(out, "flg", regs.rflags, "1;33");
    out.write(clear);
}

static void
print_rip(serial_port &out, uintptr_t addr)
{
    constexpr size_t per_line = 16;
    addr -= per_line;
    uint8_t *data = reinterpret_cast<uint8_t*>(addr);

    char bit[100];

    out.write("\n");
    for (unsigned i = 0; i < per_line*3; i += per_line) {
        util::format({bit, sizeof(bit)}, "\e[0;33m%20lx: \e[0m", addr + i);
        out.write(bit);

        for (unsigned j = 0; j < per_line; ++j) {
            util::format({bit, sizeof(bit)}, "%02x ", data[i+j]);
            out.write(bit);
        }
        out.write("\n");
    }
}

void
print_user_state(serial_port &out, const cpu_state &regs)
{
    out.write("\n\e[1;35m USER:\e[0 ");
    print_cpu_state(out, regs);

    // This will print out the bytes around RIP - useful to
    // see if corruption of the .text segment has happened.
    // This online disassembler can be useful for looking at
    // a pile of hex bytes:
    // https://onlinedisassembler.com/odaweb/
    //
    // However, normally this should be commented out, as any
    // page fault due to a jump to a bad address will also
    // cause a page fault in the panic handler.
    //
    // print_rip(out, regs.rip);
}

} // namespace panicking
