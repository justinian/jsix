PHDRS
{
    rodata PT_LOAD PHDRS FILEHDR FLAGS (4) /* read-only */;
    text PT_LOAD ;
    rwdata PT_LOAD ;
    bss PT_LOAD ;
}

MEMORY
{
    panic (rwxa) : ORIGIN = 0xFFFF800080000000, LENGTH = 256M
}

ENTRY(_panic_entry)

SECTIONS
{
    .rodata ORIGIN(panic) + SIZEOF_HEADERS : {
		 *(.rodata*)
    } :rodata

	.text ALIGN(4K) : {
		*(.text*)
	} :text

	.data ALIGN(4K) : {
		 *(.data*)
		 *(.init_array*)
	} :rwdata

	.bss ALIGN(4K) : {
		__bss_start = .;
		*(.bss*)
		__bss_end = .;
	} :bss
}
