section .bss
mymessage:
	resq 1024

extern main
extern exit

section .text
global getpid
getpid:
	push rbp
	mov rbp, rsp

	mov rax, 5  ; getpid syscall
	syscall     ; pid is now already in rax, so just return

	pop rbp
	ret

global debug
debug:
	push rbp
	mov rbp, rsp

	mov rax, 1  ; debug syscall
	syscall

	pop rbp
	ret

global sleep
sleep:
	push rbp
	mov rbp, rsp

	mov rax, 4  ; sleep syscall
	syscall

	pop rbp
	ret

global _start
_start:
	xor rbp, rbp	; Sentinel rbp
	push rbp
	push rbp
	mov rbp, rsp

	mov rdi, 0
	mov rsi, 0
	call main

	mov rdi, rax
	call exit
