; This file is part of the C standard library for the jsix operating
; system.
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at https://mozilla.org/MPL/2.0/.

struc JMPBUF
.rbx:       resq 1
.rsp:       resq 1
.rbp:       resq 1
.r12:       resq 1
.r13:       resq 1
.r14:       resq 1
.r15:       resq 1

.retaddr:   resq 1

.mxcsr:     resd 1
.fcw:       resw 1

endstruc

global setjmp: function (setjmp.end - setjmp)
setjmp:
    push rbp
    mov rbp, rsp

    mov rax, [rsp + 8]
    mov [rdi + JMPBUF.retaddr], rax

    mov [rdi + JMPBUF.rbx], rbx
    mov [rdi + JMPBUF.rsp], rsp
    mov [rdi + JMPBUF.rbp], rbp
    mov [rdi + JMPBUF.r12], r12
    mov [rdi + JMPBUF.r13], r13
    mov [rdi + JMPBUF.r14], r14
    mov [rdi + JMPBUF.r15], r15

    stmxcsr [rdi + JMPBUF.mxcsr]
    fnstcw [rdi + JMPBUF.fcw]

    mov rax, 0 ; actual setjmp returns 0

    pop rbp
    ret
.end:


global longjmp: function (longjmp.end - longjmp)
longjmp:
    push rbp
    mov rbp, rsp

    mov rbx, [rdi + JMPBUF.rbx]
    mov rbp, [rdi + JMPBUF.rbp]
    mov r12, [rdi + JMPBUF.r12]
    mov r13, [rdi + JMPBUF.r13]
    mov r14, [rdi + JMPBUF.r14]
    mov r15, [rdi + JMPBUF.r15]

    ldmxcsr [rdi + JMPBUF.mxcsr]
    fnclex
    fldcw [rdi + JMPBUF.fcw]

    mov rsp, [rdi + JMPBUF.rsp]

    mov rax, rsi
    cmp rax, 0
    jne .done
    mov rax, 1

.done:
    pop rbp
    ret
.end:
