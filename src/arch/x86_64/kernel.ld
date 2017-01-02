ENTRY(start)
SECTIONS
{
	. = 0x100000;

	.__mbHeader : {
		mboot = .;
		*(.__mbHeader)
		. = ALIGN(4096);
	}

	.text : {
		code = .;
		*(.text)
		. = ALIGN(4096);
	}

	.data : {
		 data = .;
		 *(.data)
		 *(.rodata)
		 . = ALIGN(4096);
	}

	.bss : {
		bss = .;
		*(.bss)
		. = ALIGN(4096);
	}

	end = .;
}
