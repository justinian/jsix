ENTRY(_kernel_start)
SECTIONS
{
	. = 0xFFFF800000000000;

	.header : {
		__header_start = .;
		KEEP(*(.header))
		__header_end = .;
	}

	.text ALIGN(4096) : {
		*(.text)
		*(.isrs)
	}

	.data ALIGN(4096) : {
		 *(.data)
		 *(.rodata)
	}

	.ctors : ALIGN(8) {
		 __ctors = .;
		 KEEP(*(.ctors))
		 __ctors_end = .;
	 }

	.bss ALIGN(4096) : {
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}

	.note : {
		*(.note.*)
	}

	.eh_frame : {
		__eh_frame_start = .;
		KEEP(*(.eh_frame))
		__eh_frame_end = .;
	}

	.eh_frame_hdr : {
		KEEP(*(.eh_frame_hdr))
	}

	__eh_frame_hdr_start = SIZEOF(.eh_frame_hdr) > 0 ? ADDR(.eh_frame_hdr) : 0;
	__eh_frame_hdr_end = SIZEOF(.eh_frame_hdr) > 0 ? . : 0;

	kernel_end = ALIGN(4096);
}
