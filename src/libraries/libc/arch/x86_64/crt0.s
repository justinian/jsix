extern main
extern exit
extern _init_libj6

global _start:function (_start.end - _start)
_start:
	mov rbp, rsp
	mov rdi, rsp
	call _init_libj6

	pop rdi
	mov rsi, rsp
	call main

	mov rdi, rax
	call exit
.end:
