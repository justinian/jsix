%include "push_all.inc"

extern __counter_syscall_enter
extern __counter_syscall_sysret

extern syscall_handler
extern isr_handler_return
global syscall_handler_prelude
syscall_handler_prelude:
	swapgs
	mov [gs:0x08], rsp
	mov rsp, [gs:0x00]

	push 0x23	; ss
	push rsp
	pushf
	push 0x2b	; cs
	push rcx	; user rip
	push 0		; bogus interrupt
	push 0		; bogus errorcode
	push_all_and_segments

	inc qword [rel __counter_syscall_enter]

	mov rax, [gs:0x08]
	mov [rsp + 0x98], rax
	mov rax, [rsp + 0x70]

	mov rdi, rsp
	call syscall_handler
	mov rsp, rax

	mov rax, [rsp + 0x90]
	and rax, 0x3
	cmp rax, 0x3
	jne isr_handler_return

	inc qword [rel __counter_syscall_sysret]

	swapgs

	pop_all_and_segments
	add rsp, 16	; ignore bogus interrupt / error
	pop rcx		; user rip
	add rsp, 32 ; ignore cs, flags, rsp, ss

	o64 sysret
