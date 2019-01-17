#include "kutil/assert.h"
#include "console.h"

[[noreturn]] void
__kernel_assert(const char *file, unsigned line, const char *message)
{
	console *cons = console::get();
	if (cons) {
		cons->set_color(9 , 0);
		cons->puts("\n\n       ERROR: ");
		cons->puts(file);
		cons->puts(":");
		cons->put_dec(line);
		cons->puts(":  ");
		cons->puts(message);
	}

	__asm__ ( "int $0e7h" );
	while (1) __asm__ ("hlt");
}

extern "C" [[noreturn]] void
__assert_fail(const char *message, const char *file, unsigned int line, const char *function)
{
	__kernel_assert(file, line, message);
}
