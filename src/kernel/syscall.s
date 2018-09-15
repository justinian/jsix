%include "push_all.inc"

extern syscall_handler
global syscall_handler_prelude
syscall_handler_prelude:
	push 0		; ss, doesn't matter here
	push rsp
	pushf
	push 0		; cs, doesn't matter here
	push rcx	; user rip
	push 0		; bogus interrupt
	push 0		; bogus errorcode
	push_all_and_segments

	mov rdi, rsp
	call syscall_handler
	mov rsp, rax

	pop_all_and_segments
	add rsp, 16	; ignore bogus interrupt / error
	pop rcx		; user rip
	add rsp, 32 ; ignore cs, flags, rsp, ss

	o64 sysret
