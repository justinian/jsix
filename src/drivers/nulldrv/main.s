global _start
_start:
	xor rbp, rbp	; Sentinel rbp

	pop rsi			; My PID
	mov rdi, 0		; DEBUG syscall

.loop:
	syscall
	jmp .loop

