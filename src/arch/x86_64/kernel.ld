ENTRY(_start)
SECTIONS
{
	OFFSET = 0xFFFF800000000000;
	. = OFFSET + 0x100000;

	.header : {
		__header_start = .;
		KEEP(*(.header))
		__header_end = .;
	}

	.text : {
		*(.text)
	}

	.data ALIGN(0x1000) : {
		 *(.data)
		 *(.rodata)
	}

	.isrs : {
		*(.isrs)
	}

	.bss ALIGN(0x1000) : {
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}

	.note ALIGN(0x1000) : {
		*(.note.*)
	}

	kernel_end = ALIGN(4096);
}
