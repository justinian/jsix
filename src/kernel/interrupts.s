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
%define  ISR(i, name)     EMIT_ISR name, i
%define  IRQ(i, q, name)  EMIT_IRQ name, i

section .isrs
%include "interrupt_isrs.inc"

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

extern load_process
global ramdisk_process_loader
ramdisk_process_loader:

	; create_process already pushed a cpu_state onto the stack for us, this
	; acts both as the cpu_state parameter to load_process, and the saved
	; state for the following iretq
	mov rdi, rax
	mov rsi, rbx
	call load_process

	pop_all_and_segments
	add rsp, 16		; because the ISRs add err/num
	iretq


global taskA
taskA:
	push rbp
	mov rbp, rsp
	mov rax, 0xaaaaaaaaaaaaaaaa

.loop:
	syscall
	jmp .loop

global taskAend
taskAend:
