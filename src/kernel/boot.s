MAGIC	equ  'j6KERNEL'	; jsix kernel header magic number

section .header
align 8
global _kernel_header
_kernel_header:
	dq MAGIC            ; Kernel header magic
	dw 32               ; Kernel header length
	dw 2                ; Header version 2
	dw VERSION_MAJOR    ; Kernel version
	dw VERSION_MINOR
	dw VERSION_PATCH
	dw 0                ; reserved
	dd VERSION_GITSHA
	dq 0                ; Flags

section .text
align 16
global _kernel_start:function (_kernel_start.end - _kernel_start)
global _kernel_start.real
_kernel_start:
	push rbp     ; Never executed, fake function prelude
	mov rbp, rsp ; to calm down gdb

.real:
	cli

	mov rsp, idle_stack_end
	sub rsp, 16
	mov rbp, rsp

	extern kernel_main
	call kernel_main

	; Kernel init is over, wait for the scheduler to
	; take over
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
align 0x100
idle_stack_begin:
	resb 0x4000 ; 16KiB stack space

global idle_stack_end
idle_stack_end:
