PHDRS {
	headers PT_PHDR FLAGS(4) FILEHDR PHDRS ;
	rodata PT_LOAD FLAGS(4) ;
	text PT_LOAD FLAGS(5) ;
	dynamic PT_DYNAMIC FLAGS(4) ;
	note PT_NOTE ;
	eh_frame PT_GNU_EH_FRAME ;
}

SECTIONS {
	. = SIZEOF_HEADERS;

	/DISCARD/ : {
		*(.got*)
		*(.plt*)
		*(.note.*)
		*(.hash*)
		*(.debug*)
	}

	.illegal.relocations : {
		*(.rel*)
	}

	.illegal.writeable : {
		*(.data*)
		*(.bss*)
	}

	.rodata : {
		*(.rodata*)
	} :rodata

	.dynamic : {
		*(.dynamic)
	} :dynamic

	.dynsym : {
		*(.dynsym*)
	} :rodata :dynamic

	.dynstr : {
		*(.dynstr*)
	} :rodata :dynamic

	.gnu.hash : {
		*(.gnu.hash*)
	} :rodata

	.eh_frame_hdr : {
		__eh_frame_start = .;
		KEEP(*(.eh_frame))
		__eh_frame_end = .;

		KEEP(*(.eh_frame_hdr))
	} :eh_frame

	.shstrtab : {
		*(.shstrtab)
	}

	.text ALIGN(0x1000) : {
		*(.text*)
		*(.init*)
		*(.fini*)
		. = ALIGN(0x1000);
	} :text

	ASSERT(SIZEOF(.illegal.relocations) == 0,
			"Code has introduced relocations into the VDSO")
	ASSERT(SIZEOF(.illegal.writeable) == 0,
			"Code has introduced writeable data into the VDSO")
}
