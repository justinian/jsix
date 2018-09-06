%include "push_all.inc"

global syscall_enable
syscall_enable:
	; IA32_EFER - set bit 0, syscall enable
	mov rcx, 0xc0000080
	rdmsr
	or rax, 0x1
	wrmsr

	; IA32_STAR - cs for syscall
	mov rcx, 0xc0000081
	mov rax, 0			; not used
	mov rdx, 0x00180008	; GDT:3 (user code), GDT:1 (kernel code)
	wrmsr

	; IA32_LSTAR - RIP for syscall
	mov rcx, 0xc0000082
	lea rax, [rel syscall_handler_prelude]
	mov rdx, rax
	shr rdx, 32
	wrmsr

	; IA32_FMASK - FLAGS mask inside syscall
	mov rcx, 0xc0000084
	mov rax, 0x200
	mov rdx, 0
	wrmsr

	ret

extern syscall_handler
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
