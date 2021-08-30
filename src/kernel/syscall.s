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

extern __counter_syscall_enter
extern __counter_syscall_sysret
extern syscall_registry
extern syscall_invalid


global syscall_handler_prelude:function (syscall_handler_prelude.end - syscall_handler_prelude)
syscall_handler_prelude:
	push rbp     ; Never executed, fake function prelude
	mov rbp, rsp ; to calm down gdb

.real:
	swapgs
	mov [gs:CPU_DATA.rsp3], rsp
	mov rsp, [gs:CPU_DATA.rsp0]

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
	push r11
	push r12
	push r13
	push r14
	push r15

	inc qword [rel __counter_syscall_enter]

	cmp rax, NUM_SYSCALLS
	jge .bad_syscall

	lea r11, [rel syscall_registry]
	mov r11, [r11 + rax * 8]
	cmp r11, 0
	je .bad_syscall

	call r11

	inc qword [rel __counter_syscall_sysret]
	jmp kernel_to_user_trampoline

.bad_syscall:
	mov rdi, rax
	call syscall_invalid
.end:

global kernel_to_user_trampoline:function (kernel_to_user_trampoline.end - kernel_to_user_trampoline)
kernel_to_user_trampoline:
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop rbx

	pop rbp
	pop rcx

	mov [gs:CPU_DATA.rsp0], rsp
	mov rsp, [gs:CPU_DATA.rsp3]

	swapgs
	o64 sysret
.end:

global syscall_enable:function (syscall_enable.end - syscall_enable)
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
	wrmsr

	pop rbp
	ret
.end:
