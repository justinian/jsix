%macro SYSCALL 2
	global _syscall_%1
	_syscall_%1:
		push rbp
		mov rbp, rsp

		; address of args should already be in rdi, etc
		mov rax, %2
		syscall
		; result is now already in rax, so just return

		pop rbp
		ret
%endmacro

%define SYSCALL(n, name) SYSCALL name, n
%define SYSCALL(n, name, a) SYSCALL name, n
%define SYSCALL(n, name, a, b) SYSCALL name, n
%define SYSCALL(n, name, a, b, c) SYSCALL name, n

%include "syscalls.inc"
