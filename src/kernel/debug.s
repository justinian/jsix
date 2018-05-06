
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


global get_frame
get_frame:
	mov rcx, rbp

.loop:
	mov rax, [rcx + 8]
	mov rcx, [rcx]
	cmp rdi, 0
	je .done

	sub rdi, 1
	jmp .loop

.done:
	ret
