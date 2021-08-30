%macro SYSCALL 2
	global __syscall_%1
	__syscall_%1:
		push rbp
		mov rbp, rsp

		; args should already be in rdi, etc, but rcx will
		; get stomped, so stash it in r10, which isn't a
		; callee-saved register, but also isn't used in the
		; function call ABI.
		mov r10, rcx

		mov rax, %2
		syscall
		; result is now already in rax, so just return

		pop rbp
		ret
%endmacro

{% for id, scope, method in interface.methods %}
SYSCALL {% if scope %}{{ scope.name }}_{% endif %}{{ method.name }} {{ id }}
{% endfor %}
