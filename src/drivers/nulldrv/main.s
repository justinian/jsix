global _start
_start:
	xor rbp, rbp	; Sentinel rbp

	mov rdi, 0		; DEBUG syscall

.loop:
	syscall
	jmp .loop

