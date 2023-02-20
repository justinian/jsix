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

extern _libc_crt0_start
global _start:function (_start.end - _start)
_start:
    ; No parent process exists to have created init's stack, so we create a
    ; stack in BSS and assign that to be init's first stack
    mov rsp, init_stack_top
    jmp _libc_crt0_start
.end:
