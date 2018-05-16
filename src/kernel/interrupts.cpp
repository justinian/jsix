#include <stddef.h>
#include <stdint.h>

#include "kutil/enum_bitfields.h"
#include "kutil/memory.h"
#include "console.h"
#include "device_manager.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"

enum class gdt_type : uint8_t
{
	accessed	= 0x01,
	read_write	= 0x02,
	conforming	= 0x04,
	execute		= 0x08,
	system		= 0x10,
	ring1		= 0x20,
	ring2		= 0x40,
	ring3		= 0x60,
	present		= 0x80
};
IS_BITFIELD(gdt_type);

struct gdt_descriptor
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	gdt_type type;
	uint8_t size;
	uint8_t base_high;
} __attribute__ ((packed));

struct idt_descriptor
{
	uint16_t base_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t flags;
	uint16_t base_mid;
	uint32_t base_high;
	uint32_t reserved;		// must be zero
} __attribute__ ((packed));

struct table_ptr
{
	uint16_t limit;
	uint64_t base;
} __attribute__ ((packed));

gdt_descriptor g_gdt_table[10];
idt_descriptor g_idt_table[256];
table_ptr g_gdtr;
table_ptr g_idtr;

struct registers;

extern "C" {
	void idt_write();
	void idt_load();

	void gdt_write(uint16_t cs, uint16_t ds);
	void gdt_load();

	void isr_handler(registers);
	void irq_handler(registers);

#define ISR(i, name)     extern void name ();
#define EISR(i, name)    extern void name ();
#define IRQ(i, q, name)  extern void name ();
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR
}

void idt_dump(const table_ptr &table);
void gdt_dump(const table_ptr &table);

isr
operator+(const isr &lhs, int rhs)
{
	using under_t = std::underlying_type<isr>::type;
	return static_cast<isr>(static_cast<under_t>(lhs) + rhs);
}

uint8_t
get_irq(unsigned vector)
{
	switch (vector) {
#define ISR(i, name)
#define EISR(i, name)
#define IRQ(i, q, name) case i : return q;
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR

		default: return 0xff;
	}
}

void
set_gdt_entry(uint8_t i, uint32_t base, uint64_t limit, bool is64, gdt_type type)
{
	g_gdt_table[i].limit_low = limit & 0xffff;
	g_gdt_table[i].size = (limit >> 16) & 0xf;
	g_gdt_table[i].size |= (is64 ? 0xa0 : 0xc0);

	g_gdt_table[i].base_low = base & 0xffff;
	g_gdt_table[i].base_mid = (base >> 16) & 0xff;
	g_gdt_table[i].base_high = (base >> 24) & 0xff;

	g_gdt_table[i].type = type | gdt_type::system | gdt_type::present;
}

void
set_idt_entry(uint8_t i, uint64_t addr, uint16_t selector, uint8_t flags)
{
	g_idt_table[i].base_low = addr & 0xffff;
	g_idt_table[i].base_mid = (addr >> 16) & 0xffff;
	g_idt_table[i].base_high = (addr >> 32) & 0xffffffff;
	g_idt_table[i].selector = selector;
	g_idt_table[i].flags = flags;
	g_idt_table[i].ist = 0;
	g_idt_table[i].reserved = 0;
}

static void
disable_legacy_pic()
{

	static const uint16_t PIC1 = 0x20;
	static const uint16_t PIC2 = 0xa0;

	// Mask all interrupts
	outb(0xa1, 0xff);
	outb(0x21, 0xff);

	// Start initialization sequence
	outb(PIC1, 0x11); io_wait();
	outb(PIC2, 0x11); io_wait();

	// Remap into ignore ISRs
	outb(PIC1+1, static_cast<uint8_t>(isr::isrIgnore0)); io_wait();
	outb(PIC2+1, static_cast<uint8_t>(isr::isrIgnore0)); io_wait();

	// Tell PICs about each other
	outb(PIC1+1, 0x04); io_wait();
	outb(PIC2+1, 0x02); io_wait();
}

static void
enable_serial_interrupts()
{
	uint8_t ier = inb(COM1+1);
	outb(COM1+1, ier | 0x1);
}

void
interrupts_init()
{
	kutil::memset(&g_gdt_table, 0, sizeof(g_gdt_table));
	kutil::memset(&g_idt_table, 0, sizeof(g_idt_table));

	g_gdtr.limit = sizeof(g_gdt_table) - 1;
	g_gdtr.base = reinterpret_cast<uint64_t>(&g_gdt_table);

	set_gdt_entry(1, 0, 0xfffff,  true, gdt_type::read_write | gdt_type::execute);
	set_gdt_entry(2, 0, 0xfffff,  true, gdt_type::read_write);
	set_gdt_entry(3, 0, 0xfffff,  true, gdt_type::ring3 | gdt_type::read_write | gdt_type::execute);
	set_gdt_entry(4, 0, 0xfffff,  true, gdt_type::ring3 | gdt_type::read_write);

	gdt_write(1 << 3, 2 << 3);

	g_idtr.limit = sizeof(g_idt_table) - 1;
	g_idtr.base = reinterpret_cast<uint64_t>(&g_idt_table);

#define ISR(i, name)     set_idt_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#define EISR(i, name)    set_idt_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#define IRQ(i, q, name)  set_idt_entry(i, reinterpret_cast<uint64_t>(& name), 0x08, 0x8e);
#include "interrupt_isrs.inc"
#undef IRQ
#undef EISR
#undef ISR

	idt_write();

	disable_legacy_pic();
	enable_serial_interrupts();

	log::info(logs::boot, "Interrupts enabled.");
}

struct registers
{
	uint64_t ds;
	uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
	uint64_t interrupt, errorcode;
	uint64_t rip, cs, eflags, user_esp, ss;
};

#define print_reg(name, value) cons->printf("         %s: %016lx\n", name, (value));

extern "C" uint64_t get_frame(int frame);

void
print_stacktrace(int skip = 0)
{
	console *cons = console::get();
	int frame = 0;
	uint64_t bp = get_frame(skip);
	while (bp) {
		cons->printf("    frame %2d: %lx\n", frame, bp);
		bp = get_frame(++frame + skip);
	}
}

void
isr_handler(registers regs)
{
	console *cons = console::get();

	switch (static_cast<isr>(regs.interrupt & 0xff)) {
	case isr::isrTimer:
		cons->puts("\nTICK\n");
		break;

	case isr::isrLINT0:
		cons->puts("\nLINT0\n");
		break;

	case isr::isrLINT1:
		cons->puts("\nLINT1\n");
		break;

	case isr::isrIgnore0:
	case isr::isrIgnore1:
	case isr::isrIgnore2:
	case isr::isrIgnore3:
	case isr::isrIgnore4:
	case isr::isrIgnore5:
	case isr::isrIgnore6:
	case isr::isrIgnore7:

		/*
		cons->printf("\nIGNORED PIC INTERRUPT %d\n",
				(regs.interrupt % 0xff) - 0xf0);
		*/
		break;

	case isr::isrGPFault: {
			cons->set_color(9);
			cons->puts("\nGeneral Protection Fault:\n");
			cons->set_color();

			cons->puts("       flags:");
			if (regs.errorcode & 0x01) cons->puts(" external");

			int index = (regs.errorcode & 0xf8) >> 3;
			if (index) {
				switch (regs.errorcode & 0x06) {
				case 0:
					cons->printf(" GDT[%d]\n", index);
					gdt_dump(g_gdtr);
					break;

				case 1:
				case 3:
					cons->printf(" IDT[%d]\n", index);
					idt_dump(g_idtr);
					break;

				default:
					cons->printf(" LDT[%d]??\n", index);
					break;
				}
			} else {
				cons->putc('\n');
			}

			cons->puts("\n");
			print_reg(" ds", regs.ds);
			print_reg(" cs", regs.cs);
			print_reg(" ss", regs.ss);


			cons->puts("\n");
			print_reg("rax", regs.rax);
			print_reg("rbx", regs.rbx);
			print_reg("rcx", regs.rcx);
			print_reg("rdx", regs.rdx);
			print_reg("rdi", regs.rdi);
			print_reg("rsi", regs.rsi);

			cons->puts("\n");
			print_reg("rbp", regs.rbp);
			print_reg("rsp", regs.rsp);

			cons->puts("\n");
			print_reg("rip", regs.rip);
			print_stacktrace(2);

			cons->puts("\nStack:\n");
			uint64_t sp = regs.rsp;
			while (sp <= regs.rbp) {
				cons->printf("%016x: %016x\n", sp, *reinterpret_cast<uint64_t *>(sp));
				sp += sizeof(uint64_t);
			}
		}
		while(1) asm("hlt");
		break;

	case isr::isrPageFault: {
			cons->set_color(11);
			cons->puts("\nPage Fault:\n");
			cons->set_color();

			cons->puts("       flags:");
			if (regs.errorcode & 0x01) cons->puts(" present");
			if (regs.errorcode & 0x02) cons->puts(" write");
			if (regs.errorcode & 0x04) cons->puts(" user");
			if (regs.errorcode & 0x08) cons->puts(" reserved");
			if (regs.errorcode & 0x10) cons->puts(" ip");
			cons->puts("\n");

			uint64_t cr2 = 0;
			__asm__ __volatile__ ("mov %%cr2, %0" : "=r"(cr2));
			print_reg("cr2", cr2);

			print_reg("rip", regs.rip);

			cons->puts("\n");
			print_stacktrace(2);
		}
		while(1) asm("hlt");
		break;

	case isr::isrAssert: {
			cons->set_color();

			cons->puts("\n");
			print_reg("rax", regs.rax);
			print_reg("rbx", regs.rbx);
			print_reg("rcx", regs.rcx);
			print_reg("rdx", regs.rdx);
			print_reg("rdi", regs.rdi);
			print_reg("rsi", regs.rsi);

			cons->puts("\n");
			print_reg("rbp", regs.rbp);
			print_reg("rsp", regs.rsp);

			cons->puts("\n");
			print_reg("rip", regs.rip);
			print_stacktrace(2);
		}
		while(1) asm("hlt");
		break;

	default:
		cons->set_color(9);
		cons->puts("\nReceived ISR interrupt:\n");
		cons->set_color();

		cons->printf("         ISR: %02lx\n", regs.interrupt);
		cons->printf("         ERR: %lx\n", regs.errorcode);
		cons->puts("\n");

		print_reg(" ds", regs.ds);
		print_reg("rdi", regs.rdi);
		print_reg("rsi", regs.rsi);
		print_reg("rbp", regs.rbp);
		print_reg("rsp", regs.rsp);
		print_reg("rbx", regs.rbx);
		print_reg("rdx", regs.rdx);
		print_reg("rcx", regs.rcx);
		print_reg("rax", regs.rax);
		cons->puts("\n");

		print_reg("rip", regs.rip);
		print_reg(" cs", regs.cs);
		print_reg(" ef", regs.eflags);
		print_reg("esp", regs.user_esp);
		print_reg(" ss", regs.ss);

		cons->puts("\n");
		print_stacktrace(2);
		while(1) asm("hlt");
	}

	*reinterpret_cast<uint32_t *>(0xffffff80fee000b0) = 0;
}

void
irq_handler(registers regs)
{
	console *cons = console::get();
	uint8_t irq = get_irq(regs.interrupt);
	if (! device_manager::get().dispatch_irq(irq)) {
		cons->set_color(11);
		cons->printf("\nReceived unknown IRQ: %d (vec %d)\n",
				irq, regs.interrupt);
		cons->set_color();

		print_reg(" ds", regs.ds);
		print_reg("rdi", regs.rdi);
		print_reg("rsi", regs.rsi);
		print_reg("rbp", regs.rbp);
		print_reg("rsp", regs.rsp);
		print_reg("rbx", regs.rbx);
		print_reg("rdx", regs.rdx);
		print_reg("rcx", regs.rcx);
		print_reg("rax", regs.rax);
		cons->puts("\n");

		print_reg("rip", regs.rip);
		print_reg(" cs", regs.cs);
		print_reg(" ef", regs.eflags);
		print_reg("esp", regs.user_esp);
		print_reg(" ss", regs.ss);
		while(1) asm("hlt");
	}

	*reinterpret_cast<uint32_t *>(0xffffff80fee000b0) = 0;
}


void
gdt_dump(const table_ptr &table)
{
	console *cons = console::get();
	cons->printf("         GDT: loc:%lx size:%d\n", table.base, table.limit+1);

	int count = (table.limit + 1) / sizeof(gdt_descriptor);
	const gdt_descriptor *gdt =
		reinterpret_cast<const gdt_descriptor *>(table.base);

	for (int i = 0; i < count; ++i) {
		uint32_t base =
			(gdt[i].base_high << 24) |
			(gdt[i].base_mid << 16) |
			gdt[i].base_low;

		uint32_t limit =
			static_cast<uint32_t>(gdt[i].size & 0x0f) << 16 |
			gdt[i].limit_low;

		cons->printf("          %02d:", i);
		if (! bitfield_has(gdt[i].type, gdt_type::present)) {
			cons->puts(" Not Present\n");
			continue;
		}

		cons->printf(" Base %08x  limit %05x ", base, limit);

		switch (gdt[i].type & gdt_type::ring3) {
			case gdt_type::ring3: cons->puts("ring3"); break;
			case gdt_type::ring2: cons->puts("ring2"); break;
			case gdt_type::ring1: cons->puts("ring1"); break;
			default: cons->puts("ring0"); break;
		}

		cons->printf(" %s %s %s %s %s %s %s\n",
			bitfield_has(gdt[i].type, gdt_type::accessed) ? "A" : " ",
			bitfield_has(gdt[i].type, gdt_type::read_write) ? "RW" : "  ",
			bitfield_has(gdt[i].type, gdt_type::conforming) ? "C" : " ",
			bitfield_has(gdt[i].type, gdt_type::execute) ? "EX" : "  ",
			bitfield_has(gdt[i].type, gdt_type::system) ? "S" : " ",
			(gdt[i].size & 0x80) ? "KB" : " B",
			(gdt[i].size & 0x60) == 0x20 ? "64" :
				(gdt[i].size & 0x60) == 0x40 ? "32" : "16");
	}
}

void
idt_dump(const table_ptr &table)
{
	log::info(logs::boot, "Loaded IDT at: %lx size: %d bytes", table.base, table.limit+1);

	int count = (table.limit + 1) / sizeof(idt_descriptor);
	const idt_descriptor *idt =
		reinterpret_cast<const idt_descriptor *>(table.base);

	for (int i = 0; i < count; ++i) {
		uint64_t base =
			(static_cast<uint64_t>(idt[i].base_high) << 32) |
			(static_cast<uint64_t>(idt[i].base_mid)  << 16) |
			idt[i].base_low;

		char const *type;
		switch (idt[i].flags & 0xf) {
			case 0x5: type = " 32tsk "; break;
			case 0x6: type = " 16int "; break;
			case 0x7: type = " 16trp "; break;
			case 0xe: type = " 32int "; break;
			case 0xf: type = " 32trp "; break;
			default:  type = " ????? "; break;
		}

		if (idt[i].flags & 0x80) {
			log::debug(logs::boot,
					"   Entry %3d: Base:%lx Sel(rpl %d, ti %d, %3d) IST:%d %s DPL:%d", i, base,
					(idt[i].selector & 0x3),
					((idt[i].selector & 0x4) >> 2),
					(idt[i].selector >> 3),
					idt[i].ist,
					type,
					((idt[i].flags >> 5) & 0x3));
		}
	}
}
