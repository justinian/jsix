extern g_idtr
extern g_gdtr

global idt_write
idt_write:
	lidt [g_idtr]
	ret

global idt_load
idt_load:
	sidt [g_idtr]
	ret

global gdt_write
gdt_write:
	lgdt [g_gdtr]
	ret

global gdt_load
gdt_load:
	sgdt [g_gdtr]
	ret

%macro ISR_NOERRCODE 1
	global isr%1
	isr%1:
		cli
		push byte 0
		push byte %1
		jmp isr_handler_prelude
%endmacro

%macro ISR_ERRCODE 1
	global isr%1
	isr%1:
		cli
		push byte %1
		jmp isr_handler_prelude
%endmacro

ISR_NOERRCODE  0
ISR_NOERRCODE  1
ISR_NOERRCODE  2
ISR_NOERRCODE  3
ISR_NOERRCODE  4
ISR_NOERRCODE  5
ISR_NOERRCODE  6
ISR_NOERRCODE  7
ISR_ERRCODE    8
ISR_NOERRCODE  9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

extern isr_handler
global isr_handler_prelude
isr_handler_prelude:
	push rax
	push rcx
	push rdx
	push rbx
	push rsp
	push rbp
	push rsi
	push rdi

	mov ax, ds		; Save the data segment register
	push rax

	mov ax, 0x10	; load the kernel data segment
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call isr_handler

	pop rax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	pop rdi
	pop rsi
	pop rbp
	pop rsp
	pop rbx
	pop rdx
	pop rcx
	pop rax

	add rsp, 16		; because the ISRs added err/num
	sti
	iretq
