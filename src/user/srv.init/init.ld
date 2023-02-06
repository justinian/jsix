PHDRS
{
    rodata PT_LOAD PHDRS FILEHDR FLAGS (4) /* read-only */;
    text PT_LOAD ;
    rwdata PT_LOAD ;
    bss PT_LOAD ;
}

SECTIONS
{
    . = 0x20000000 + SIZEOF_HEADERS ;

    .rodata : {
		 *(.rodata*)
    } :rodata

	.text ALIGN(4K) : {
		*(.text*)
	} :text

	.data ALIGN(4K) : {
		 *(.data*)
	} :rwdata

    .init_array : {
		 *(.init_array*)
    } :rwdata

    .got : {
		 *(.got*)
    } :rwdata

	.bss ALIGN(4K) : {
		__bss_start = .;
		*(.bss*)
		__bss_end = .;
	} :bss

    /DISCARD/ : {
        *(.gcc_except_table*)
        *(.eh_frame*)
    }
}
