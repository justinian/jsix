%include "push_all.inc"
%include "tasking.inc"

; Make sure to keep MAX_SYSCALLS in sync with
; syscall::MAX in syscall.h
MAX_SYSCALLS equ 64

extern __counter_syscall_enter
extern __counter_syscall_sysret

extern syscall_registry
extern syscall_invalid

global syscall_handler_prelude
syscall_handler_prelude:
	swapgs
	mov [gs:CPU_DATA.rsp3], rsp
	mov rsp, [gs:CPU_DATA.rsp0]

	push rcx
	push rbp
	mov rbp, rsp
	push r11

	inc qword [rel __counter_syscall_enter]

	cmp rax, MAX_SYSCALLS
	jle .ok_syscall

	mov rdi, rax
	call syscall_invalid

.ok_syscall:
	lea r11, [rel syscall_registry]
	mov r11, [r11 + rax * 8]
	call r11

	inc qword [rel __counter_syscall_sysret]

	pop r11
	pop rbp
	pop rcx

	mov [gs:CPU_DATA.rsp0], rsp
	mov rsp, [gs:CPU_DATA.rsp3]

	swapgs
	o64 sysret
