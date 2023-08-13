extern ldso_init
extern _GLOBAL_OFFSET_TABLE_

global _ldso_start:function hidden (_ldso_start.end - _ldso_start)
_ldso_start:
    mov rbp, rsp

    ; Save off anything that might be a function arg
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9

    ; Call ldso_init with the loader-provided stack data and
    ; also the address of the GOT, since clang refuses to take
    ; the address of it, only dereference it.
    mov rdi, rbp
    lea rsi, [rel _GLOBAL_OFFSET_TABLE_]
    call ldso_init
    ; The real program's entrypoint is now in rax

    ; Put the function call params back
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi

    jmp rax
.end: