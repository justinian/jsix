; boot.s -- Kernel start location.

[BITS 64]
ALIGN 4

SECTION .text
[GLOBAL start]

start:
	; Load multiboot header location
	mov ebx, 0xdeadbeef
	jmp $
