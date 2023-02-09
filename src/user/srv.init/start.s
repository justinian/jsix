extern main
extern exit
extern __init_libj6
extern __init_libc
extern _arg_modules_phys

section .bss
align 0x100
init_stack_start:
    resb 0x8000 ; 16KiB stack space
init_stack_top:

section .text

global _start:function (_start.end - _start)
_start:
    ; No parent process exists to have created init's stack, so we create a
    ; stack in BSS and assign that to be init's first stack
    mov [_arg_modules_phys], rdi
    mov rsp, init_stack_top
    push 0
    push 0

    mov rbp, rsp
    mov rdi, rsp
    call __init_libj6
    call __init_libc

    pop rdi
    mov rsi, rsp
    call main

    mov rdi, rax
    call exit
.end:
