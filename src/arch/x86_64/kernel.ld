ENTRY(_start)
SECTIONS
{
	. = 0x100000;

	.header : {
		header = .;
		KEEP(*(.header))
	}

	.text : {
		code = .;
		*(.text)
	}

	.data ALIGN(0x1000) : {
		 data = .;
		 *(.data)
		 *(.rodata)
	}

	.bss ALIGN(0x1000) : {
		bss = .;
		*(.bss)
	}

	.note ALIGN(0x1000) : {
		*(.note.*)
	}

	kernel_end = ALIGN(4096);
}
