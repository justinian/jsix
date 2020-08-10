#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "gdt.h"
#include "objects/process.h"
#include "objects/thread.h"
#include "page_manager.h"
#include "symbol_table.h"

size_t __counter_syscall_enter = 0;
size_t __counter_syscall_sysret = 0;

void
print_regs(const cpu_state &regs)
{
	console *cons = console::get();

	uint64_t cr2 = 0;
	__asm__ __volatile__ ("mov %%cr2, %0" : "=r"(cr2));

	cons->printf("       process: %llx", bsp_cpu_data.p->koid());
	cons->printf("   thread: %llx\n", bsp_cpu_data.t->koid());

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
	print_regM("rsp", regs.user_rsp);
	print_regR("sp0", bsp_cpu_data.rsp0);

	print_regL("rip", regs.rip);
	print_regM("cr3", page_manager::get()->get_pml4());
	print_regR("cr2", cr2);

	cons->puts("\n");
}

struct frame
{
	frame *prev;
	uintptr_t return_addr;
};

void
print_stacktrace(int skip)
{
	console *cons = console::get();
	symbol_table *syms = symbol_table::get();

	frame *fp = nullptr;
	int fi = -skip;
	__asm__ __volatile__ ( "mov %%rbp, %0" : "=r" (fp) );

	while (fp && fp->return_addr) {
		if (fi++ >= 0) {
			const symbol_table::entry *e = syms ? syms->find_symbol(fp->return_addr) : nullptr;
			const char *name = e ? e->name : "";
			cons->printf("  frame %2d:", fi-1);

			cons->set_color(5);
			cons->printf(" %016llx", fp->return_addr);
			cons->set_color();

			cons->set_color(6);
			cons->printf("  %s\n", name);
			cons->set_color();
		}
		fp = fp->prev;
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

