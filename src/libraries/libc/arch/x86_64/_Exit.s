extern _syscall_process_exit
global _PDCLIB_Exit
_PDCLIB_Exit:
	; arg should already be in rdi
	jmp _syscall_process_exit
