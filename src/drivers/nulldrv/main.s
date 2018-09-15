global _start
_start:
	xor rbp, rbp	; Sentinel rbp
	mov r11, 0		; counter

.loop:
	mov rax, 1		; DEBUG syscall
	;mov rax, 0		; NOOP syscall
	;syscall
	int 0xee

	inc r11
	cmp r11, 3

	jle .loop

	mov rax, 3		; PAUSE syscall
	; syscall
	int 0xee

	mov r11, 0
	jmp .loop
