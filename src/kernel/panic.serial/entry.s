%include "push_all.inc"

section .text

extern panic_handler

global _panic_entry
_panic_entry:
	cli
	push 0 ; NMI doesn't push an error code
	push 2 ; NMI is int 2
	push_all
	check_swap_gs

	mov r9, rsp

	mov rax, [rsp + REGS.rip]
	push rax

	jmp panic_handler


global _current_gsbase
_current_gsbase:
	mov rax, [gs:0]
	ret
