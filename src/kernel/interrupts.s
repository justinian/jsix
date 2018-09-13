%include "push_all.inc"

extern isr_handler
global isr_handler_prelude
isr_handler_prelude:
	push_all_and_segments

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

	mov rdi, rsp
	call irq_handler
	mov rsp, rax

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
%define UISR(i, name)     EMIT_ISR name, i
%define  ISR(i, name)     EMIT_ISR name, i
%define  IRQ(i, q, name)  EMIT_IRQ name, i

section .isrs
%include "interrupt_isrs.inc"
