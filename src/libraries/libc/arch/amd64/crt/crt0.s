extern _GLOBAL_OFFSET_TABLE_

extern main
extern exit
extern __init_libj6
extern __init_libc

; Put the address of the given symbol in rax
; This macro is the same as in util/got.inc,
; but crt0 can't have a dep on libutil
%macro lookup_GOT 1
    lea rax, [rel _GLOBAL_OFFSET_TABLE_]
    mov rax, [rax + %1 wrt ..got]
%endmacro

global _start:function weak (_libc_crt0_start.end - _libc_crt0_start)
global _libc_crt0_start:function (_libc_crt0_start.end - _libc_crt0_start)

_start:
_libc_crt0_start:
    mov rdx, [rsp]      ; grab args pointer

    push 0              ; Add null frame
    push 0
    mov rbp, rsp

    lookup_GOT __init_libj6 
    call rax
    mov rbx, rax

    lookup_GOT __init_libc
    call rax

    mov rdi, 0
    mov rsi, rsp
    mov rdx, 0    ; TODO: actually parse stack for argc, argv, envp
    mov rcx, rbx
    lookup_GOT main
    call rax

    mov rdi, rax
    lookup_GOT exit
    call rax
.end:
