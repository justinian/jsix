struc TCB
.rsp:          resq 1
.rsp0:         resq 1
.rsp3:         resq 1
.rflags3:      resq 1
.pml4:         resq 1
endstruc

struc CPU_DATA
.self:         resq 1
.id:           resw 1
.index:        resw 1
.reserved      resd 1
.rsp0:         resq 1
.rsp3:         resq 1
.rflags3:      resq 1
.tcb:          resq 1
.thread:       resq 1
.process:      resq 1
.idt:          resq 1
.tss:          resq 1
.gdt:          resq 1
endstruc

struc TSS
.res0:         resd 1
.rsp0:         resq 1
.rsp1:         resq 1
.rsp2:         resq 1
.ist0:         resq 1
.ist1:         resq 1
.ist2:         resq 1
.ist3:         resq 1
.ist4:         resq 1
.ist5:         resq 1
.ist6:         resq 1
.ist7:         resq 1
.res1:         resq 1
.res2:         resw 1
.iomap:        resw 1
endstruc


; vim: ft=asm
