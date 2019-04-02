%include "push_all.inc"

extern load_process_image

global ramdisk_process_loader
ramdisk_process_loader:

	; create_process already pushed a cpu_state onto the stack for us, this
	; acts both as the cpu_state parameter to load_process_image, and the
	; saved state for the following iretq

	pop rdi ; the address of the program image
	pop rsi ; the size of the program image
	pop rdx ; the address of this process' process structure

	call load_process_image

	push rax ; load_process_image returns the process entrypoint

	swapgs
	iretq

