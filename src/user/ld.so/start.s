extern ldso_init
extern ldso_plt_lookup
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
    mov rdi, [rbp]
    lea rsi, [rel _GLOBAL_OFFSET_TABLE_]
    call ldso_init

    ; The real program's entrypoint is now in rax, save it to r11
    mov r11, rax

    ; Put the function call params back
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi

    ; Pop all the loader args
    pop rsp ; Point the stack at the first arg
    mov rax, 0
    mov rbx, 0
.poploop:
    mov eax, [dword rsp]    ; size
    mov ebx, [dword rsp+4]  ; type
    add rsp, rax
    cmp ebx, 0
    jne .poploop

    mov rbp, rsp
    jmp r11
.end:


global _ldso_plt_lookup:function hidden (_ldso_plt_lookup.end - _ldso_plt_lookup)
_ldso_plt_lookup:
    pop rax ; image struct address
    pop r11 ; jmprel entry index

    ; Save off anything that might be a function arg
    push rdi
    push rsi
    push rdx
    push rcx
    push r8
    push r9

    mov rdi, rax
    mov rsi, r11
    call ldso_plt_lookup
    ; The function's address is now in rax

    ; Put the function call params back
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi

    jmp rax
.end:
