global get_mxcsr: function hidden (get_mxcsr.end - get_mxcsr)
get_mxcsr:
    push 0
    stmxcsr [rsp]
    pop rax
    ret
.end:

global set_mxcsr: function hidden (set_mxcsr.end - set_mxcsr)
set_mxcsr:
    push rdi
    ldmxcsr [rsp]
    pop rax
    ret
.end:

global get_xcr0: function hidden (get_xcr0.end - get_xcr0)
get_xcr0:
    xor rcx, rcx ; there is no dana there is only xcr0
    xgetbv
    ret ; technically edx has the high 32 bits, but bits 10+ are reserved
.end:

global set_xcr0: function hidden (set_xcr0.end - set_xcr0)
set_xcr0:
    xor rcx, rcx ; there is no dana there is only xcr0
    mov rax, rdi ; technically edx should be or'd into the high bits, but xcr0 bits 10+ are resereved
    xsetbv
    ret
.end:

global get_rsp: function hidden (get_rsp.end - get_rsp)
get_rsp:
	mov rax, rsp
	ret
.end:

global get_rip: function hidden (get_rip.end - get_rip)
get_rip:
	pop rax ; do the same thing as 'ret', except with 'jmp'
	jmp rax ; with the return address still in rax
.end:

global get_caller: function hidden (get_caller.end - get_caller)
get_caller:
	; No prelude - don't touch rsp or rbp
	mov rax, [rbp+8]
	ret
.end:

global get_grandcaller: function hidden (get_grandcaller.end - get_grandcaller)
get_grandcaller:
	; No prelude - don't touch rsp or rbp
	mov rax, [rbp]
	mov rax, [rax+8]
	ret
.end:

global get_gsbase: function hidden (get_gsbase.end - get_gsbase)
get_gsbase:
	rdgsbase rax
	ret
.end:

global _halt: function hidden (_halt.end - _halt)
_halt:
	hlt
	jmp _halt
.end:

