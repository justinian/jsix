global _start
_start:
	xor rbp, rbp	; Sentinel rbp

.loop:
	;mov rax, 1		; DEBUG syscall
	mov rax, 0		; NOOP syscall
	syscall

	jmp .loop

