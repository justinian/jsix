#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "gdt.h"

#define print_reg(name, value) cons->printf("         %s: %016lx\n", name, (value));

void
print_regs(const cpu_state &regs)
{
	console *cons = console::get();

	print_reg("rax", regs.rax);
	print_reg("rbx", regs.rbx);
	print_reg("rcx", regs.rcx);
	print_reg("rdx", regs.rdx);
	print_reg("rdi", regs.rdi);
	print_reg("rsi", regs.rsi);

	cons->puts("\n");
	print_reg(" r8", regs.r8);
	print_reg(" r9", regs.r9);
	print_reg("r10", regs.r10);
	print_reg("r11", regs.r11);
	print_reg("r12", regs.r12);
	print_reg("r13", regs.r13);
	print_reg("r14", regs.r14);
	print_reg("r15", regs.r15);

	cons->puts("\n");
	print_reg("rbp", regs.rbp);
	print_reg("rsp", regs.user_rsp);
	print_reg("sp0", tss_get_stack(0));

	cons->puts("\n");
	print_reg(" ds", regs.ds);
	print_reg(" cs", regs.cs);
	print_reg(" ss", regs.ss);

	cons->puts("\n");
	print_reg("rip", regs.rip);
}

void
print_stacktrace(int skip)
{
	console *cons = console::get();
	int frame = 0;
	uint64_t bp = get_frame(skip);
	while (bp) {
		cons->printf("    frame %2d: %lx\n", frame, bp);
		bp = get_frame(++frame + skip);
	}
}

void
print_stack(const cpu_state &regs)
{
	console *cons = console::get();

	cons->puts("\nStack:\n");
	uint64_t sp = regs.user_rsp;
	while (sp <= regs.rbp) {
		cons->printf("%016x: %016x\n", sp, *reinterpret_cast<uint64_t *>(sp));
		sp += sizeof(uint64_t);
	}
}

