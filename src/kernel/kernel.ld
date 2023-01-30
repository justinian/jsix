PHDRS
{
    rodata PT_LOAD PHDRS FILEHDR FLAGS (4) /* read-only */;
    text PT_LOAD ;
    rwdata PT_LOAD ;
    bss PT_LOAD ;
}

MEMORY
{
    kernel (rwxa) : ORIGIN = 0xFFFF800000000000, LENGTH = 256M
}

ENTRY(_kernel_start.real)

SECTIONS
{
	.header ORIGIN(kernel) + SIZEOF_HEADERS : {
		HIDDEN(__header_start = .);
		KEEP(*(.header))
		HIDDEN(__header_end = .);
	} :rodata

    .rodata : {
		 *(.rodata*)
    } :rodata

    .ap_startup : {
		 *(.ap_startup*)
    } :rodata

	.ctors ALIGN(8) : {
		 HIDDEN(__ctors = .);
		 KEEP(*(.ctors))
		 KEEP(*(.init_array))
		 HIDDEN(__ctors_end = .);
	 } :rodata

	.text ALIGN(4K) : {
		*(.text*)
		KEEP(*(.isrs))
	} :text

	.data ALIGN(4K) : {
		 *(.data*)
	} :rwdata

    .syscall_registry : {
        *(.syscall_registry*)
    } :rwdata

	.bss ALIGN(4K) : {
		HIDDEN(__bss_start = .);
		*(.bss*)
		HIDDEN(__bss_end = .);
	} :bss

	HIDDEN(__kernel_end = ALIGN(4096));
}
