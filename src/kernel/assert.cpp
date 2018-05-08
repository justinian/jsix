#include "kutil/assert.h"
#include "console.h"

[[noreturn]] void
__kernel_assert(const char *file, unsigned line, const char *message)
{
	console *cons = console::get();
	if (cons) {
		cons->set_color(9 , 0);
		cons->puts("\n\n  ERROR: ");
		cons->puts(file);
		cons->puts(":");
		cons->put_dec(line);
		cons->puts(":  ");
		cons->puts(message);
	}

	__asm__ __volatile__( 
		"movq %0, %%r8;"
		"movq %1, %%r9;"
		"movq %2, %%r10;"
		"movq $0, %%rdx;"
		"divq %%rdx;"
		: // no outputs
		: "r"((uint64_t)line), "r"(file), "r"(message)
		: "rax", "rdx", "r8", "r9", "r10");

	while (1);
}
