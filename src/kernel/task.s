%include "tasking.inc"

extern xcr0_val

global task_switch: function hidden (task_switch.end - task_switch)
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
	mov r15, [gs:CPU_DATA.tcb]     ; r15: current task TCB
	mov [r15 + TCB.rsp], rsp

	; Copy off saved user rsp
	mov rcx, [gs:CPU_DATA.rsp3]    ; rcx: current task's saved user rsp
	mov [r15 + TCB.rsp3], rcx

	; Copy off saved user rflags
	mov rcx, [gs:CPU_DATA.rflags3] ; rcx: current task's saved user rflags
	mov [r15 + TCB.rflags3], rcx

    ; Save processor extended state
    mov rcx, [r15 + TCB.xsave]     ; rcx: current task's XSAVE area
    cmp rcx, 0
    jz .xsave_done

    mov rax, [rel xcr0_val]
    mov rdx, rax
    shl rdx, 32
    xsave [rcx]
.xsave_done:

	; Install next task's TCB
	mov [gs:CPU_DATA.tcb], rdi     ; rdi: next TCB (function param)
	mov rsp, [rdi + TCB.rsp]       ; next task's stack pointer
	mov r14, 0x00003fffffffffff
	and r14, [rdi + TCB.pml4]      ; r14: next task's pml4 (phys portion of address)

	; Update syscall/interrupt rsp
	mov rcx, [rdi + TCB.rsp0]      ; rcx: top of next task's kernel stack
	mov [gs:CPU_DATA.rsp0], rcx

	mov rdx, [gs:CPU_DATA.tss]     ; rdx: address of TSS
	mov [rdx + TSS.rsp0], rcx

	; Update saved user rsp
	mov rcx, [rdi + TCB.rsp3]      ; rcx: new task's saved user rsp
	mov [gs:CPU_DATA.rsp3], rcx

    ; Load processor extended state
    mov rcx, [rdi + TCB.xsave]     ; rcx: new task's XSAVE area
    cmp rcx, 0
    jz .xrstor_done

    mov rax, [rel xcr0_val]
    mov rdx, rax
    shl rdx, 32
    xrstor [rcx]
.xrstor_done:

	; Update saved user rflags
	mov rcx, [rdi + TCB.rflags3]   ; rcx: new task's saved user rflags
	mov [gs:CPU_DATA.rflags3], rcx

	; check if we need to update CR3
	mov rdx, cr3                   ; rdx: old CR3
	cmp r14, rdx
	je .no_cr3
	mov cr3, r14
.no_cr3:

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx

	pop rbp
	ret
.end:

global _current_gsbase: function hidden (_current_gsbase.end - _current_gsbase)
_current_gsbase:
	mov rax, [gs:CPU_DATA.self]
	ret
.end:
