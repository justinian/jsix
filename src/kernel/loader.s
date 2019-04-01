%include "push_all.inc"

extern load_process

global ramdisk_process_loader
ramdisk_process_loader:
	; create_process already pushed a cpu_state onto the stack for us, this
	; acts both as the cpu_state parameter to load_process, and the saved
	; state for the following iretq

	pop rdi ; the address of the program image
	pop rsi ; the size of the program image
	pop rdx ; the address of this process' process structure
	pop rcx ; the cpu_state
	call load_process

	swapgs

	xor rax, rax
	mov ax, ss
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	pop_all
	add rsp, 16		; because the ISRs add err/num
	iretq


