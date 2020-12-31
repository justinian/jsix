extern main
extern exit
extern _init_libc

global _start:function (_start.end - _start)
_start:
	mov rbp, rsp
	mov rdi, rsp

	call _init_libc

	pop rdi
	mov rsi, rsp

	call main

	mov rdi, rax
	call exit
.end:
