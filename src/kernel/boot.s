MAGIC	equ  'j6KERNEL'	; jsix kernel header magic number

section .header
align 8
global _kernel_header: data hidden
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
global _kernel_start: function hidden (_kernel_start.end - _kernel_start)
global _kernel_start.real: function hidden
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

    ; Kernel init is over, fall through to bsp_idle and wait for
    ; the scheduler to take over
.end:

global bsp_idle: function hidden (bsp_idle.end - bsp_idle)
bsp_idle:
	hlt
	jmp bsp_idle
.end:

global interrupts_enable: function hidden
interrupts_enable:
	sti
	ret

global interrupts_disable: function hidden
interrupts_disable:
	cli
	ret

section .bss
align 0x100
idle_stack_begin:
	resb 0x4000 ; 16KiB stack space

global idle_stack_end: data hidden
idle_stack_end:
