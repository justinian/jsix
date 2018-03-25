#include "vga.h"

void do_the_set_registers();

void kernel_main() {
	volatile register int foo = 0x1a1b1c10;
	volatile register int bar = 0;

	terminal_initialize(5);
	terminal_writestring("YES HELLO THIS IS KERNEL");

	do_the_set_registers();
}
