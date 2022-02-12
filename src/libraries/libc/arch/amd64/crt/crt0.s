extern main
extern exit
extern __init_libj6
extern __init_libc

global _start:function (_start.end - _start)
_start:
	mov rbp, rsp
	mov rdi, rsp
	call __init_libj6
	call __init_libc

	pop rdi
	mov rsi, rsp
	call main

	mov rdi, rax
	call exit
.end:
