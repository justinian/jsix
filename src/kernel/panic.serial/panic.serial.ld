ENTRY(_panic_entry)
SECTIONS
{
	. = 0xFFFF800080000000;

	.text ALIGN(4096) : {
		*(.text*)
	}

	.data ALIGN(4096) : {
		 *(.data*)
		 *(.rodata*)
	}

	.bss ALIGN(4096) : {
		__bss_start = .;
		*(.bss*)
		__bss_end = .;
	}
}
