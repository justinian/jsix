
section .text
global do_the_set_registers
do_the_set_registers:
	mov rax, 0xdeadbeef0badc0de
	mov r8, rcx
	mov r9, rdi

global _halt
_halt:
	hlt
	jmp _halt
