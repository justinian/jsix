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

	push 0x23   ; ss
	push 0x00   ; rsp - to be filled
	push r11    ; rflags
	push 0x2b   ; cs
	push rcx    ; user rip
	push 0      ; bogus error
	push 0      ; bogus vector
	push_all

	inc qword [rel __counter_syscall_enter]

	mov rax, [gs:0x08]
	mov [rsp + 0xa0], rax
	mov rax, [rsp + 0x70]

	mov rdi, rsp
	call syscall_handler
	mov rsp, rax

	mov rax, [rsp + 0x90]
	and rax, 0x3
	cmp rax, 0x3
	jne isr_handler_return

	inc qword [rel __counter_syscall_sysret]

	mov rax, [rsp + 0xa0]
	mov [gs:0x08], rax

	pop_all
	add rsp, 16 ; ignore bogus interrupt / error
	pop rcx     ; user rip
	add rsp, 8  ; ignore cs
	pop r11     ; flags
	add rsp, 16 ; rsp, ss

	mov [gs:0x00], rsp
	mov rsp, [gs:0x08]

	swapgs
	o64 sysret
