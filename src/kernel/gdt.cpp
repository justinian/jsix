#include <stdint.h>

#include "kutil/assert.h"
#include "kutil/enum_bitfields.h"
#include "kutil/memory.h"
#include "console.h"
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

struct tss_descriptor
{
	uint16_t limit_low;
	uint16_t base_00;
	uint8_t base_16;
	gdt_type type;
	uint8_t size;
	uint8_t base_24;
	uint32_t base_32;
	uint32_t reserved;
} __attribute__ ((packed));

struct tss_entry
{
	uint32_t reserved0;

	uint64_t rsp[3]; // stack pointers for CPL 0-2
	uint64_t ist[8]; // ist[0] is reserved

	uint64_t reserved1;
	uint16_t reserved2;
	uint16_t iomap_offset;
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
tss_entry g_tss;


extern "C" {
	void idt_write();
	void idt_load();

	void gdt_write(uint16_t cs, uint16_t ds, uint16_t tr);
	void gdt_load();
}

void
gdt_set_entry(uint8_t i, uint32_t base, uint64_t limit, bool is64, gdt_type type)
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
tss_set_entry(uint8_t i, uint64_t base, uint64_t limit)
{
	tss_descriptor tssd;
	tssd.limit_low = limit & 0xffff;
	tssd.size = (limit >> 16) & 0xf;

	tssd.base_00 = base & 0xffff;
	tssd.base_16 = (base >> 16) & 0xff;
	tssd.base_24 = (base >> 24) & 0xff;
	tssd.base_32 = (base >> 32) & 0xffffffff;
	tssd.reserved = 0;

	tssd.type =
		gdt_type::accessed |
		gdt_type::execute |
		gdt_type::ring3 |
		gdt_type::present;
	kutil::memcpy(&g_gdt_table[i], &tssd, sizeof(tss_descriptor));
}

void
idt_set_entry(uint8_t i, uint64_t addr, uint16_t selector, uint8_t flags)
{
	g_idt_table[i].base_low = addr & 0xffff;
	g_idt_table[i].base_mid = (addr >> 16) & 0xffff;
	g_idt_table[i].base_high = (addr >> 32) & 0xffffffff;
	g_idt_table[i].selector = selector;
	g_idt_table[i].flags = flags;
	g_idt_table[i].ist = 0;
	g_idt_table[i].reserved = 0;
}

void
tss_set_stack(int ring, addr_t rsp)
{
	kassert(ring < 3, "Bad ring passed to tss_set_stack.");
	g_tss.rsp[ring] = rsp;
}

addr_t
tss_get_stack(int ring)
{
	kassert(ring < 3, "Bad ring passed to tss_get_stack.");
	return g_tss.rsp[ring];
}

void
gdt_init()
{
	kutil::memset(&g_gdt_table, 0, sizeof(g_gdt_table));
	kutil::memset(&g_idt_table, 0, sizeof(g_idt_table));

	g_gdtr.limit = sizeof(g_gdt_table) - 1;
	g_gdtr.base = reinterpret_cast<uint64_t>(&g_gdt_table);

	// Kernel CS/SS - always 64bit
	gdt_set_entry(1, 0, 0xfffff,  true, gdt_type::read_write | gdt_type::execute);
	gdt_set_entry(2, 0, 0xfffff,  true, gdt_type::read_write);

	// User CS32/SS/CS64 - layout expected by SYSRET
	gdt_set_entry(3, 0, 0xfffff,  false, gdt_type::ring3 | gdt_type::read_write | gdt_type::execute);
	gdt_set_entry(4, 0, 0xfffff,  true, gdt_type::ring3 | gdt_type::read_write);
	gdt_set_entry(5, 0, 0xfffff,  true, gdt_type::ring3 | gdt_type::read_write | gdt_type::execute);

	kutil::memset(&g_tss, 0, sizeof(tss_entry));
	g_tss.iomap_offset = sizeof(tss_entry);

	addr_t tss_base = reinterpret_cast<addr_t>(&g_tss);

	// Note that this takes TWO GDT entries
	tss_set_entry(6, tss_base, sizeof(tss_entry));

	gdt_write(1 << 3, 2 << 3, 6 << 3);

	g_idtr.limit = sizeof(g_idt_table) - 1;
	g_idtr.base = reinterpret_cast<uint64_t>(&g_idt_table);

	idt_write();
}

void
gdt_dump()
{
	const table_ptr &table = g_gdtr;

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
idt_dump()
{
	const table_ptr &table = g_idtr;

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
