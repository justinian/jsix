extern main
extern exit
extern __init_libj6
extern _arg_modules_phys

section .bss
align 0x100
init_stack_start:
	resb 0x8000 ; 16KiB stack space
init_stack_top:

section .text

global _start:function (_start.end - _start)
_start:

	; No parent process exists to have created init's stack, so we override
	; _start to deal with that in two ways:

	;  1. We create a stack in BSS and assign that to be init's first stack
	;  2. We take advantage of the fact that rsp is useless here as a way
	;     for the kernel to tell init where its initial modules page is.
	mov [_arg_modules_phys], rsp
	mov rsp, init_stack_top
	push 0
	push 0

 	mov rbp, rsp
	mov rdi, rsp
	call __init_libj6

	pop rdi
	mov rsi, rsp
	call main

	mov rdi, rax
	call exit
.end:
