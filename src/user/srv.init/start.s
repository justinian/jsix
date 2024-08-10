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

    ; Push fake initial stack
    push 0x00       ; stack sentinel (16 bytes)
    push 0x00       ;
    push 0x00       ; alignment padding
    push 0x00       ; auxv sentinel (16 bytes)
    push 0x00       ;
    push 0x00       ; envp sentinel (8 bytes)
    push 0x00       ; argv sentinel (8 bytes)
    push rsi        ; argv[1] -- for init, not actually a char*
    push rdi        ; argv[0] -- for init, not actually a char*
    push 0x02       ; argc

    jmp _libc_crt0_start
.end:
