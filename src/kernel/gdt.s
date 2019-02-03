extern g_idtr
extern g_gdtr

global idt_write
idt_write:
	lidt [rel g_idtr]
	ret

global idt_load
idt_load:
	sidt [rel g_idtr]
	ret

global gdt_write
gdt_write:
	lgdt [rel g_gdtr]
	mov ax, si ; second arg is data segment
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	push qword rdi ; first arg is code segment
	lea rax, [rel .next]
	push rax
	o64 retf
.next:
	ltr dx
	ret

global gdt_load
gdt_load:
	sgdt [rel g_gdtr]
	ret

