section .bss
mymessage:
	resq 1024

extern main
extern exit

section .text

%macro SYSCALL 2
	global %1
	%1:
		push rbp
		mov rbp, rsp

		; address of args should already be in rdi, etc
		mov rax, %2
		syscall
		; result is now already in rax, so just return

		pop rbp
		ret
%endmacro

SYSCALL system_log, 0x00
SYSCALL object_wait, 0x09
SYSCALL process_koid, 0x10
SYSCALL thread_koid, 0x18
SYSCALL thread_create, 0x19
SYSCALL thread_exit, 0x1a
SYSCALL thread_pause, 0x1b
SYSCALL thread_sleep, 0x1c
SYSCALL channel_create, 0x20
SYSCALL channel_close, 0x21
SYSCALL channel_send, 0x22
SYSCALL channel_receive, 0x23

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
