struc REGS
.r15           resq 1  ; 0x00
.r14           resq 1  ; 0x08
.r13           resq 1  ; 0x10
.r12           resq 1  ; 0x18
.r11           resq 1  ; 0x20
.r10           resq 1  ; 0x28
.r9            resq 1  ; 0x30
.r8            resq 1  ; 0x38

.rdi           resq 1  ; 0x40
.rsi           resq 1  ; 0x48
.rbp           resq 1  ; 0x50
.rbx           resq 1  ; 0x58
.rdx           resq 1  ; 0x60
.rcx           resq 1  ; 0x68
.rax           resq 1  ; 0x70

.int           resq 1  ; 0x78
.err           resq 1  ; 0x80
.rip           resq 1  ; 0x88
.cs3           resq 1  ; 0x90
.rflags        resq 1  ; 0x98
.rsp3          resq 1  ; 0xa0
.ss3           resq 1  ; 0xa8
endstruc

regs_total_size equ 0xb0
regs_extra_size equ 0x78

%macro push_all 0
	sub rsp, regs_extra_size

	mov [rsp + REGS.rax], rax
	mov [rsp + REGS.rcx], rcx
	mov [rsp + REGS.rdx], rdx
	mov [rsp + REGS.rbx], rbx
	mov [rsp + REGS.rbp], rbp
	mov [rsp + REGS.rsi], rsi
	mov [rsp + REGS.rdi], rdi

	mov [rsp + REGS.r8 ], r8
	mov [rsp + REGS.r9 ], r9
	mov [rsp + REGS.r10], r10
	mov [rsp + REGS.r11], r11
	mov [rsp + REGS.r12], r12
	mov [rsp + REGS.r13], r13
	mov [rsp + REGS.r14], r14
	mov [rsp + REGS.r15], r15
%endmacro

%macro pop_all 0
	mov rax, [rsp + REGS.rax]
	mov rcx, [rsp + REGS.rcx]
	mov rdx, [rsp + REGS.rdx]
	mov rbx, [rsp + REGS.rbx]
	mov rbp, [rsp + REGS.rbp]
	mov rsi, [rsp + REGS.rsi]
	mov rdi, [rsp + REGS.rdi]

	mov r8,  [rsp + REGS.r8 ]
	mov r9,  [rsp + REGS.r9 ]
	mov r10, [rsp + REGS.r10]
	mov r11, [rsp + REGS.r11]
	mov r12, [rsp + REGS.r12]
	mov r13, [rsp + REGS.r13]
	mov r14, [rsp + REGS.r14]
	mov r15, [rsp + REGS.r15]

	add rsp, regs_extra_size
%endmacro

%macro check_swap_gs 0
	mov rax, [rsp+0x90]
	and rax, 0x03 ; mask out the RPL
	cmp rax, 0x03
	jne %%noswapgs
	swapgs
%%noswapgs:
%endmacro

; vim: ft=asm
