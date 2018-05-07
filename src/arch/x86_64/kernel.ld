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

	.data : {
		 *(.data)
		 *(.rodata)
	}

	.isrs : {
		*(.isrs)
	}

	.note : {
		*(.note.*)
	}

	.bss ALIGN(0x1000) : {
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}

	kernel_end = ALIGN(4096);
}
