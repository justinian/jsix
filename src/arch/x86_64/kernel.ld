ENTRY(kernel_main)
SECTIONS
{
	. = 0x100000;

	.text : {
		code = .;
		*(.text.entry)
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

	end = ALIGN(4096);
}
