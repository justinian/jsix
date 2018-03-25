
section .text
global do_the_set_registers
do_the_set_registers:
	mov rax, 0xdeadbeef0badc0de
	mov rbx, 0xdeadbeef0badc0de
	mov rcx, 0xdeadbeef0badc0de
	mov rdx, 0xdeadbeef0badc0de

global _halt
_halt:
	hlt
	jmp _halt
