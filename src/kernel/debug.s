global get_rsp
get_rsp:
	mov rax, rsp
	ret

global get_rip
get_rip:
	pop rax ; do the same thing as 'ret', except with 'jmp'
	jmp rax ; with the return address still in rax

global get_gsbase
get_gsbase:
	rdgsbase rax
	ret

global _halt
_halt:
	hlt
	jmp _halt

