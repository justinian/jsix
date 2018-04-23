MAGIC	equ  0x600db007			; Popcorn OS header magic number

section .header
align 4
global _header
_header:
	dd MAGIC			; Kernel header magic
	dw 1				; Header version 1
	dw 1				; Kernel header length
	db VERSION_MAJOR	; Kernel version
	db VERSION_MINOR
	dw VERSION_PATCH
	dd VERSION_GITSHA
	dq _start			; Kernel entrypoint

section .text
align 16
global _start:function (_start.end - _start)
_start:
	cli

	mov rsp, stack_end
	extern kernel_main
	call kernel_main

	cli

.hang:
	hlt
	jmp .hang
.end:

global interrupts_enable
interrupts_enable:
	sti
	ret

global interrupts_disable
interrupts_disable:
	cli
	ret

section .bss
stack_begin:
	resb 0x4000 ; 16KiB stack space
stack_end:
