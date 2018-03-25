MAGIC	equ  0x600db007			; Popcorn OS header magic number

section .header
align 4
global _header
_header:
	dd MAGIC
	db VERSION_MAJOR
	db VERSION_MINOR
	dw VERSION_PATCH
	dd VERSION_GITSHA
	dq _start

section .text
align 16
global _start:function (_start.end - _start)
_start:
	extern kernel_main
	call kernel_main

	cli

.hang:
	hlt
	jmp .hang
.end:
