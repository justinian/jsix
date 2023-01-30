%include "tasking.inc"

global task_switch: function hidden
task_switch:
	push rbp
	mov rbp, rsp

	; Save the rest of the callee-saved regs
	push rbx
	push r12
	push r13
	push r14
	push r15

	; Update previous task's TCB
	mov rax, [gs:CPU_DATA.tcb]     ; rax: current task TCB
	mov [rax + TCB.rsp], rsp

	; Copy off saved user rsp
	mov rcx, [gs:CPU_DATA.rsp3]    ; rcx: current task's saved user rsp
	mov [rax + TCB.rsp3], rcx

	; Copy off saved user rflags
	mov rcx, [gs:CPU_DATA.rflags3] ; rcx: current task's saved user rflags
	mov [rax + TCB.rflags3], rcx

	; Install next task's TCB
	mov [gs:CPU_DATA.tcb], rdi     ; rdi: next TCB (function param)
	mov rsp, [rdi + TCB.rsp]       ; next task's stack pointer
	mov rax, 0x00003fffffffffff
	and rax, [rdi + TCB.pml4]      ; rax: next task's pml4 (phys portion of address)

	; Update syscall/interrupt rsp
	mov rcx, [rdi + TCB.rsp0]      ; rcx: top of next task's kernel stack
	mov [gs:CPU_DATA.rsp0], rcx

	mov rdx, [gs:CPU_DATA.tss]     ; rdx: address of TSS
	mov [rdx + TSS.rsp0], rcx

	; Update saved user rsp
	mov rcx, [rdi + TCB.rsp3]      ; rcx: new task's saved user rsp
	mov [gs:CPU_DATA.rsp3], rcx

	; Update saved user rflags
	mov rcx, [rdi + TCB.rflags3]   ; rcx: new task's saved user rflags
	mov [gs:CPU_DATA.rflags3], rcx

	; check if we need to update CR3
	mov rdx, cr3                   ; rdx: old CR3
	cmp rax, rdx
	je .no_cr3
	mov cr3, rax
.no_cr3:

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx

	pop rbp
	ret

global _current_gsbase: function hidden
_current_gsbase:
	mov rax, [gs:CPU_DATA.self]
	ret
