#include "kutil/assert.h"
#include "console.h"

[[noreturn]] void
__kernel_assert(const char *file, unsigned line, const char *message)
{
	console *cons = console::get();
	if (cons) {
		cons->set_color(9 , 0);
		cons->puts("\n\n       ERROR: ");
		cons->puts(message);
		cons->puts("\n              ");
		cons->puts(file);
		cons->puts(":");
		cons->put_dec(line);
		cons->puts("\n");
	}

	__asm__ ( "int $0xe4" );
	while (1) __asm__ ("hlt");
}
