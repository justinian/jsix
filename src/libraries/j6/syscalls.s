%macro SYSCALL 2
	global j6_%1
	j6_%1:
		push rbp
		mov rbp, rsp

		; args should already be in rdi, etc, but rcx will
		; get stomped, so stash it in r10, which isn't a
		; callee-saved register, but also isn't used in the
		; function call ABI.
		mov r10, rcx

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
%define SYSCALL(n, name, a, b, c, d) SYSCALL name, n
%define SYSCALL(n, name, a, b, c, d, e) SYSCALL name, n

%include "j6/tables/syscalls.inc"
