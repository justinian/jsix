%include "push_all.inc"

extern load_process

global ramdisk_process_loader
ramdisk_process_loader:

	; create_process already pushed a cpu_state onto the stack for us, this
	; acts both as the cpu_state parameter to load_process, and the saved
	; state for the following iretq
	mov rdi, rax
	mov rsi, rbx
	call load_process

	pop_all_and_segments
	add rsp, 16		; because the ISRs add err/num
	iretq


