%include "push_all.inc"

extern load_process_image

global preloaded_process_init
preloaded_process_init:

	; create_process already pushed the arguments for load_process_image and
	; the following iretq onto the stack for us

	pop rdi ; a pointer to the program descriptor
	call load_process_image

	; user rsp is now in rax, put it in the right place for iret
	mov [rsp + 0x18], rax

	; the entrypoint should already be on the stack
	swapgs
	iretq

