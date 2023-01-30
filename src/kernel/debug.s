global get_rsp: function hidden
get_rsp:
	mov rax, rsp
	ret

global get_rip: function hidden
get_rip:
	pop rax ; do the same thing as 'ret', except with 'jmp'
	jmp rax ; with the return address still in rax

global get_caller: function hidden
get_caller:
	; No prelude - don't touch rsp or rbp
	mov rax, [rbp+8]
	ret

global get_grandcaller: function hidden
get_grandcaller:
	; No prelude - don't touch rsp or rbp
	mov rax, [rbp]
	mov rax, [rax+8]
	ret

global get_gsbase: function hidden
get_gsbase:
	rdgsbase rax
	ret

global _halt: function hidden
_halt:
	hlt
	jmp _halt

