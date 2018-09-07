global _start
_start:
	xor rbp, rbp	; Sentinel rbp

.loop:
	mov rax, 0		; DEBUG syscall
	syscall

	jmp .loop

