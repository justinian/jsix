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

