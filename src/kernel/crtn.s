section .init
	; Control flow falls through to here from other .init sections
	pop rbp
	ret

section .fini
	; Control flow falls through to here from other .fini sections
	pop rbp
	ret

