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

; vim: ft=asm
