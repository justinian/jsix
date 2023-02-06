
global idt_write: function hidden (idt_write.end - idt_write)
idt_write:
	lidt [rdi] ; first arg is the IDT pointer location
	ret
.end:

global idt_load: function hidden (idt_load.end - idt_load)
idt_load:
	sidt [rdi] ; first arg is where to write the idtr value
	ret
.end:

global gdt_write: function hidden (gdt_write.end - gdt_write)
gdt_write:
	lgdt [rdi] ; first arg is the GDT pointer location

	mov ax, dx ; third arg is data segment
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	push qword rsi ; second arg is code segment
	lea rax, [rel .next]
	push rax
	o64 retf
.next:
	ltr cx ; fourth arg is the TSS
	ret
.end:

global gdt_load: function hidden (gdt_load.end - gdt_load)
gdt_load:
	sgdt [rdi] ; first arg is where to write the gdtr value
	ret
.end:

