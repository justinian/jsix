%include "push_all.inc"

extern isr_handler
global isr_handler_prelude
isr_handler_prelude:
	push_all
	check_swap_gs

	mov rdi, rsp
	mov rsi, rsp
	call isr_handler
	jmp isr_handler_return

extern irq_handler
global irq_handler_prelude
irq_handler_prelude:
	push_all
	check_swap_gs

	mov rdi, rsp
	mov rsi, rsp
	call irq_handler
	; fall through to isr_handler_return

global isr_handler_return
isr_handler_return:
	check_swap_gs
	pop_all

	add rsp, 16		; because the ISRs added err/num
	iretq

%macro EMIT_ISR 2
	global %1
	%1:
		push 0
		push %2
		jmp isr_handler_prelude
%endmacro

%macro EMIT_EISR 2
	global %1
	%1:
		push %2
		jmp isr_handler_prelude
%endmacro

%macro EMIT_IRQ 2
	global %1
	%1:
		push 0
		push %2
		jmp irq_handler_prelude
%endmacro

%define EISR(i, s, name)     EMIT_EISR name, i   ; ISR with error code
%define  ISR(i, s, name)     EMIT_ISR name, i
%define  IRQ(i, q, name)  EMIT_IRQ name, i

section .isrs
%include "interrupt_isrs.inc"
