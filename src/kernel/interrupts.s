extern g_idtr
extern g_gdtr

global idt_write
idt_write:
	lidt [rel g_idtr]
	ret

global idt_load
idt_load:
	sidt [rel g_idtr]
	ret

global gdt_write
gdt_write:
	lgdt [rel g_gdtr]
	mov ax, si ; second arg is data segment
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	push qword rdi ; first arg is code segment
	lea rax, [rel .next]
	push rax
	o64 retf
.next:
	ltr dx
	ret

global gdt_load
gdt_load:
	sgdt [rel g_gdtr]
	ret

%macro push_all_and_segments 0
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi

	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds
	push rax
%endmacro

%macro pop_all_and_segments 0
	pop rax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8

	pop rdi
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop rcx
	pop rax
%endmacro

%macro load_kernel_segments 0
	mov ax, 0x10	; load the kernel data segment
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
%endmacro

extern isr_handler
global isr_handler_prelude
isr_handler_prelude:
	push_all_and_segments
	;load_kernel_segments

	mov rdi, rsp
	call isr_handler
	mov rsp, rax

	pop_all_and_segments

	add rsp, 16		; because the ISRs added err/num
	sti
	iretq

extern irq_handler
global irq_handler_prelude
irq_handler_prelude:
	push_all_and_segments
	load_kernel_segments

	call irq_handler

	pop_all_and_segments

	add rsp, 16		; because the ISRs added err/num
	sti
	iretq

%macro EMIT_ISR 2
	global %1
	%1:
		cli
		push 0
		push %2
		jmp isr_handler_prelude
%endmacro

%macro EMIT_EISR 2
	global %1
	%1:
		cli
		push %2
		jmp isr_handler_prelude
%endmacro

%macro EMIT_IRQ 2
	global %1
	%1:
		cli
		push 0
		push %2
		jmp irq_handler_prelude
%endmacro

%define EISR(i, name)     EMIT_EISR name, i
%define  ISR(i, name)     EMIT_ISR name, i
%define  IRQ(i, q, name)  EMIT_IRQ name, i

section .isrs
%include "interrupt_isrs.inc"
