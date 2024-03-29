%include "tasking.inc"
%include "syscalls.inc"

; SYSCALL/SYSRET control MSRs
MSR_STAR   equ 0xc0000081
MSR_LSTAR  equ 0xc0000082
MSR_FMASK  equ 0xc0000084

; IA32_STAR - high 32 bits contain k+u CS
; Kernel CS: GDT[1] ring 0 bits[47:32]
;   User CS: GDT[3] ring 3 bits[63:48]
STAR_HIGH  equ \
	(((1 << 3) | 0)) | \
	(((3 << 3) | 3) << 16)

; IA32_FMASK - Mask off interrupts in syscalls
FMASK_VAL  equ 0x200

extern __counter_syscall_enter          ; syscall.cpp.cog
extern __counter_syscall_sysret         ;
extern syscall_registry                 ;
extern syscall_invalid                  ;
extern cpu_initialize_thread_state      ; cpu.cpp


global syscall_handler_prelude: function hidden (syscall_handler_prelude.end - syscall_handler_prelude)
syscall_handler_prelude:
	push rbp     ; Never executed, fake function prelude
	mov rbp, rsp ; to calm down gdb

.real:
	swapgs
	mov [gs:CPU_DATA.rsp3], rsp
	mov rsp, [gs:CPU_DATA.rsp0]
	mov [gs:CPU_DATA.rflags3], r11

	push rcx
	push rbp
	mov rbp, rsp

	; account for the hole in the sysv abi
	; argument list since SYSCALL uses rcx.
	; r10 is non-preserved but not part of
	; the function call ABI, so the rcx arg
	; was stashed there.
	mov rcx, r10

	push rbx
	push r12
	push r13
	push r14
	push r15

	; if we've got more than 6 arguments, the rest
	; are on the user stack, and pointed to by rbx.
	; push rbx so that it's the 7th argument.
	push rbx

	inc qword [rel __counter_syscall_enter]

	cmp rax, NUM_SYSCALLS
	jge .bad_syscall

	lea r11, [rel syscall_registry]
	mov r11, [r11 + rax * 8]
	cmp r11, 0
	je .bad_syscall

	call r11

	add rsp, 8 ; account for passing rbx on the stack

	inc qword [rel __counter_syscall_sysret]
	jmp kernel_to_user_trampoline

.bad_syscall:
	mov rdi, rax
	call syscall_invalid
.end:

global initialize_user_cpu: function hidden (initialize_user_cpu.end - initialize_user_cpu)
initialize_user_cpu:
    call cpu_initialize_thread_state

    mov rax, 0xaaaaaaaa
    mov rdx, 0xdddddddd
    mov r8,  0x08080808
    mov r9,  0x09090909
    mov r10, 0x10101010
    mov r11, 0x11111111

    pop rdi
    pop rsi

    ; fall through to kernel_to_user_trampoline
.end:

global kernel_to_user_trampoline: function hidden (kernel_to_user_trampoline.end - kernel_to_user_trampoline)
kernel_to_user_trampoline:
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx

	pop rbp
	pop rcx

	mov r11, [gs:CPU_DATA.tss]
	mov [r11 + TSS.rsp0], rsp
	mov [gs:CPU_DATA.rsp0], rsp
	mov rsp, [gs:CPU_DATA.rsp3]
	mov r11, [gs:CPU_DATA.rflags3]

	swapgs
	o64 sysret
.end:

global syscall_enable: function hidden (syscall_enable.end - syscall_enable)
syscall_enable:
	push rbp
	mov rbp, rsp

	mov rcx, MSR_STAR
	mov rax, 0
	mov rdx, STAR_HIGH
	wrmsr

	mov rcx, MSR_LSTAR
	mov rax, syscall_handler_prelude.real
	mov rdx, rax
	shr rdx, 32
	wrmsr

	mov rcx, MSR_FMASK
	mov rax, FMASK_VAL
	mov rdx, 0
	wrmsr

	pop rbp
	ret
.end:
