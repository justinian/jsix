section .init
global _init:function
_init:
	push rbp
	mov rbp, rsp
	; Control flow falls through to other .init sections

section .fini
global _fini:function
_fini:
	push rbp
	mov rbp, rsp
	; Control flow falls through to other .fini sections

