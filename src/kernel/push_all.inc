%macro push_all 0
	sub rsp, 0x78

	mov [rsp + 0x70], rax
	mov [rsp + 0x68], rcx
	mov [rsp + 0x60], rdx
	mov [rsp + 0x58], rbx
	mov [rsp + 0x50], rbp
	mov [rsp + 0x48], rsi
	mov [rsp + 0x40], rdi

	mov [rsp + 0x38], r8
	mov [rsp + 0x30], r9
	mov [rsp + 0x28], r10
	mov [rsp + 0x20], r11
	mov [rsp + 0x18], r12
	mov [rsp + 0x10], r13
	mov [rsp + 0x08], r14
	mov [rsp + 0x00], r15
%endmacro

%macro pop_all 0
	mov rax, [rsp + 0x70]
	mov rcx, [rsp + 0x68]
	mov rdx, [rsp + 0x60]
	mov rbx, [rsp + 0x58]
	mov rbp, [rsp + 0x50]
	mov rsi, [rsp + 0x48]
	mov rdi, [rsp + 0x40]

	mov r8, [rsp + 0x38]
	mov r9, [rsp + 0x30]
	mov r10, [rsp + 0x28]
	mov r11, [rsp + 0x20]
	mov r12, [rsp + 0x18]
	mov r13, [rsp + 0x10]
	mov r14, [rsp + 0x08]
	mov r15, [rsp + 0x00]

	add rsp, 0x78
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
