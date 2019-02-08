global _start
_start:
	xor rbp, rbp	; Sentinel rbp

	mov rax, 5		; GETPID syscall
	int 0xee
	mov r12, rax	; save pid to r12

	mov rax, 1		; DEBUG syscall
	int 0xee

	cmp r12, 1
	je .dosend
	jne .doreceive

.preloop:
	mov r11, 0		; counter
	mov rbx, 20		; sleep timeout

.loop:
	mov rax, 2		; MESSAGE syscall
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

.dosend:
	mov rax, 6      ; SEND syscall
	mov rdi, 2      ; target is pid 2
	int 0xee
	jmp .preloop

.doreceive:
	mov rax, 7      ; RECEIVE syscall
	mov rdi, 1      ; source is pid 2
	int 0xee
	jmp .preloop
