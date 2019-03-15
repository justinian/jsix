section .bss
mypid: resq 1
mychild: resq 1

section .text
global _start
_start:
	xor rbp, rbp	; Sentinel rbp

	mov rax, 5		; GETPID syscall
	syscall ; int 0xee
	mov [mypid], rax

	mov rax, 8		; FORK syscall
	syscall ; int 0xee
	mov [mychild], rax

	mov r12, [mypid]
	mov r13, [mychild]
	mov rax, 1		; DEBUG syscall
	syscall ; int 0xee

	cmp r13, 0
	je .doexit

	cmp r12, 1
	je .dosend
	jne .doreceive

.preloop:
	mov r11, 0		; counter
	mov rbx, 20		; sleep timeout

.loop:
	mov rax, 1		; MESSAGE syscall
	;mov rax, 0		; NOOP syscall
	;syscall
	syscall ; int 0xee

	inc r11
	cmp r11, 2

	jle .loop

	mov rax, 4		; SLEEP syscall
	; syscall
	syscall ; int 0xee

	add rbx, 20

	mov r11, 0
	jmp .loop

.dosend:
	mov rax, 6      ; SEND syscall
	mov rdi, 2      ; target is pid 2
	syscall ; int 0xee
	jmp .preloop

.doreceive:
	mov rax, 7      ; RECEIVE syscall
	mov rdi, 1      ; source is pid 2
	syscall ; int 0xee
	jmp .preloop

.doexit
	mov rax, 9      ; EXIT syscall
	syscall
