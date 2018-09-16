global _start
_start:
	xor rbp, rbp	; Sentinel rbp
	mov r11, 0		; counter
	mov rbx, 20		; sleep timeout

.loop:
	mov rax, 1		; DEBUG syscall
	;mov rax, 0		; NOOP syscall
	;syscall
	int 0xee

	inc r11
	cmp r11, 2

	jle .loop

	mov rax, 4		; SLEEP syscall
	; syscall
	int 0xee

	add rbx, 20

	mov r11, 0
	jmp .loop
