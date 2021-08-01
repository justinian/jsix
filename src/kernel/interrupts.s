%include "push_all.inc"

section .text

extern isr_handler
global isr_handler_prelude:function (isr_handler_prelude.end - isr_handler_prelude)
isr_handler_prelude:
	push rbp     ; Never executed, fake function prelude
	mov rbp, rsp ; to calm down gdb

.real:
	push_all
	check_swap_gs

	mov rdi, rsp
	mov rsi, rsp
	call isr_handler
	jmp isr_handler_return
.end:

extern irq_handler
global irq_handler_prelude:function (irq_handler_prelude.end - irq_handler_prelude)
irq_handler_prelude:
	push rbp     ; Never executed, fake function prelude
	mov rbp, rsp ; to calm down gdb

.real:
	push_all
	check_swap_gs

	mov rdi, rsp
	mov rsi, rsp
	call irq_handler
	; fall through to isr_handler_return
.end:

global isr_handler_return:function (isr_handler_return.end - isr_handler_return)
isr_handler_return:
	check_swap_gs
	pop_all

	add rsp, 16		; because the ISRs added err/num
	iretq
.end:

%macro EMIT_ISR 2
	global %1:function (%1.end - %1)
	%1:
		push 0
		push %2
		jmp isr_handler_prelude.real
	.end:
%endmacro

%macro EMIT_EISR 2
	global %1:function (%1.end - %1)
	%1:
		push %2
		jmp isr_handler_prelude.real
	.end:
%endmacro

%macro EMIT_IRQ 2
	global %1:function (%1.end - %1)
	%1:
		push 0
		push %2
		jmp irq_handler_prelude.real
	.end:
%endmacro

%define EISR(i, s, name)     EMIT_EISR name, i   ; ISR with error code
%define  ISR(i, s, name)     EMIT_ISR name, i
%define  IRQ(i, q, name)     EMIT_IRQ name, i

%define NISR(i, s, name) ; We don't emit a handler for NMI

section .isrs
%include "interrupt_isrs.inc"
