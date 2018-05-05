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
	push rsp
	push rbp
	push rsi
	push rdi

	mov ax, ds
	push rax
%endmacro

%macro pop_all_and_segments 0
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
	load_kernel_segments

	call isr_handler

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

%include "interrupt_isrs.inc"
