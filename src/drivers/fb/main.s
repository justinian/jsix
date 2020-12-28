section .bss
mymessage:
	resq 1024

extern main
extern _init_libc
extern exit

section .text

global _start
_start:
	mov rbp, rsp

	mov rdi, rsp
	call _init_libc

	mov rdi, 0
	mov rsi, 0

	call main

	mov rdi, rax
	call exit
