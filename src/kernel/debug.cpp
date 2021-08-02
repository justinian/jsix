#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "gdt.h"
#include "objects/process.h"
#include "objects/thread.h"

size_t __counter_syscall_enter = 0;
size_t __counter_syscall_sysret = 0;

void
print_regs(const cpu_state &regs)
{
    console *cons = console::get();
    cpu_data &cpu = current_cpu();

    uint64_t cr2 = 0;
    __asm__ __volatile__ ("mov %%cr2, %0" : "=r"(cr2));

    uintptr_t cr3 = 0;
    __asm__ __volatile__ ( "mov %%cr3, %0" : "=r" (cr3) );

    cons->printf("       process: %llx", cpu.process->koid());
    cons->printf("   thread: %llx\n", cpu.thread->koid());

    print_regL("rax", regs.rax);
    print_regM("rbx", regs.rbx);
    print_regR("rcx", regs.rcx);
    print_regL("rdx", regs.rdx);
    print_regM("rdi", regs.rdi);
    print_regR("rsi", regs.rsi);

    cons->puts("\n");
    print_regL(" r8", regs.r8);
    print_regM(" r9", regs.r9);
    print_regR("r10", regs.r10);
    print_regL("r11", regs.r11);
    print_regM("r12", regs.r12);
    print_regR("r13", regs.r13);
    print_regL("r14", regs.r14);
    print_regM("r15", regs.r15);

    cons->puts("\n\n");
    print_regL("rbp", regs.rbp);
    print_regM("rsp", regs.rsp);
    print_regR("sp0", cpu.rsp0);

    print_regL("rip", regs.rip);
    print_regM("cr3", cr3);
    print_regR("cr2", cr2);

    cons->puts("\n");
}

struct frame
{
    frame *prev;
    uintptr_t return_addr;
};

void
print_stack(const cpu_state &regs)
{
    console *cons = console::get();

    cons->puts("\nStack:\n");
    uint64_t sp = regs.rsp;
    while (sp <= regs.rbp) {
        cons->printf("%016x: %016x\n", sp, *reinterpret_cast<uint64_t *>(sp));
        sp += sizeof(uint64_t);
    }
}

