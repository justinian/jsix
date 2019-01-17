ENTRY(_start)
SECTIONS
{
	OFFSET = 0xFFFFFF0000000000;
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
