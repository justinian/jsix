; This file is part of the C standard library for the jsix operating
; system.
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

extern _GLOBAL_OFFSET_TABLE_

extern main
extern exit
extern __init_libj6
extern __init_libc

extern __preinit_array_start
extern __preinit_array_end
extern __init_array_start
extern __init_array_end

global __run_ctor_list:function hidden (__run_ctor_list.end - __run_ctor_list)
__run_ctor_list:
    push rbp
    mov rbp, rsp

    push rbx
    push r12
    mov rbx, rdi
    mov r12, rsi
.start:
    cmp rbx, r12
    je .fin
    mov rax, [rbx]
    call rax
    add rbx, 8
    jmp .start
.fin:

    pop r12
    pop rbx
    pop rbp
    ret
.end:

global __run_global_ctors:function hidden (__run_global_ctors.end - __run_global_ctors)
__run_global_ctors:
    push rbp
    mov rbp, rsp

    lea rdi, [rel __preinit_array_start]
    lea rsi, [rel __preinit_array_end]
    call __run_ctor_list
    lea rdi, [rel __init_array_start]
    lea rsi, [rel __init_array_end]
    call __run_ctor_list

    pop rbp
    ret
.end:


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
    mov r15, rsp        ; grab initial stack pointer

    push 0              ; Add null frame
    push 0
    mov rbp, rsp

    mov rdi, r15
    lookup_GOT __init_libj6 
    call rax
    mov rbx, rax

    lookup_GOT __init_libc
    call rax

    call __run_global_ctors

    mov rdi, [r15]
    mov rsi, r15
    add rsi, 8
    mov rdx, 0    ; TODO: actually parse stack for argc, argv, envp
    mov rcx, rbx
    lookup_GOT main
    call rax

    mov rdi, rax
    lookup_GOT exit
    call rax
.end:
