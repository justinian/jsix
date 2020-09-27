%include "push_all.inc"
%include "tasking.inc"

; Make sure to keep MAX_SYSCALLS in sync with
; syscall::MAX in syscall.h
MAX_SYSCALLS equ 0x40

extern __counter_syscall_enter
extern __counter_syscall_sysret

extern syscall_registry
extern syscall_invalid

global syscall_handler_prelude
global syscall_handler_prelude.return
syscall_handler_prelude:
	swapgs
	mov [gs:CPU_DATA.rsp3], rsp
	mov rsp, [gs:CPU_DATA.rsp0]

	push rcx
	push rbp
	mov rbp, rsp

	; account for the hole in the sysv abi
	; argument list since SYSCALL uses rcx
	mov rcx, r8
	mov r8, r9

	push rbx
	push r11
	push r12
	push r13
	push r14
	push r15

	inc qword [rel __counter_syscall_enter]

	cmp rax, MAX_SYSCALLS
	jle .ok_syscall

.bad_syscall:
	mov rdi, rax
	call syscall_invalid

.ok_syscall:
	lea r11, [rel syscall_registry]
	mov r11, [r11 + rax * 8]
	cmp r11, 0
	je .bad_syscall

	call r11

	inc qword [rel __counter_syscall_sysret]

.return:
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop rbx

	pop rbp
	pop rcx

	mov [gs:CPU_DATA.rsp0], rsp
	mov rsp, [gs:CPU_DATA.rsp3]

	swapgs
	o64 sysret
