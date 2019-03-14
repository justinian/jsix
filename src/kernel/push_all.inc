%macro push_all_and_segments 0
	; ss        ; rsp + a8
	; rsp       ; rsp + a0
	; rflags    ; rsp + 98
	; cs        ; rsp + 90
	; rip       ; rsp + 88
	; error     ; rsp + 80
	; vector    ; rsp + 78

	push rax    ; rsp + 70
	push rcx    ; rsp + 68
	push rdx    ; rsp + 60
	push rbx    ; rsp + 58
	push rbp    ; rsp + 50
	push rsi    ; rsp + 48
	push rdi    ; rsp + 40

	push r8     ; rsp + 38
	push r9     ; rsp + 30
	push r10    ; rsp + 28
	push r11    ; rsp + 20
	push r12    ; rsp + 18
	push r13    ; rsp + 10
	push r14    ; rsp + 08
	push r15    ; rsp + 00
%endmacro

%macro pop_all_and_segments 0
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
