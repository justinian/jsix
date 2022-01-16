#include <string.h>

#include <util/no_construct.h>

#include "cpu.h"
#include "idt.h"
#include "logger.h"

extern "C" {
    void idt_write(const void *idt_ptr);

#define ISR(i, s, name)  extern void name ();
#define NISR(i, s, name)
#define EISR(i, s, name) extern void name ();
#define IRQ(i, q, name)  extern void name ();
#include "interrupt_isrs.inc"
#undef IRQ
#undef NISR
#undef EISR
#undef ISR
}

// The BSP's IDT is initialized _before_ global constructors are called,
// so we don't want it to have a global constructor, lest it overwrite
// the previous initialization.
static util::no_construct<IDT> __g_bsp_idt_storage;
IDT &g_bsp_idt = __g_bsp_idt_storage.value;
void (*__nmi_handler)();

void
IDT::set_nmi_handler(uintptr_t address)
{
    __nmi_handler = reinterpret_cast<void(*)()>(address);
}

IDT::IDT()
{
    memset(this, 0, sizeof(IDT));
    m_ptr.limit = sizeof(m_entries) - 1;
    m_ptr.base = &m_entries[0];

#define ISR(i, s, name)  set(i, & name, 0x08, 0x8e);
#define EISR(i, s, name) set(i, & name, 0x08, 0x8e);
#define IRQ(i, q, name)  set(i, & name, 0x08, 0x8e);
#define NISR(i, s, name) set(i, __nmi_handler, 0x08, 0x8e);
#include "interrupt_isrs.inc"
#undef NISR
#undef IRQ
#undef EISR
#undef ISR
}

IDT &
IDT::current()
{
    cpu_data &cpu = current_cpu();
    return *cpu.idt;
}

void
IDT::install() const
{
    idt_write(static_cast<const void*>(&m_ptr));
}

void
IDT::add_ist_entries()
{
#define ISR(i, s, name)   if (s) { set_ist(i, s); }
#define NISR(i, s, name)  if (s) { set_ist(i, s); }
#define EISR(i, s, name)  if (s) { set_ist(i, s); }
#define IRQ(i, q, name)
#include "interrupt_isrs.inc"
#undef IRQ
#undef NISR
#undef EISR
#undef ISR
}

uint8_t
IDT::used_ist_entries() const
{
    constexpr uint8_t entries = 

#define ISR(i, s, name)   ((s) ? (1 << s) : 0) |
#define NISR(i, s, name)  ((s) ? (1 << s) : 0) |
#define EISR(i, s, name)  ((s) ? (1 << s) : 0) |
#define IRQ(i, q, name)
#include "interrupt_isrs.inc"
#undef IRQ
#undef NISR
#undef EISR
#undef ISR

    0;

    return entries;
}

void
IDT::set(uint8_t i, void (*handler)(), uint16_t selector, uint8_t flags)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(handler);

    m_entries[i].base_low = addr & 0xffff;
    m_entries[i].base_mid = (addr >> 16) & 0xffff;
    m_entries[i].base_high = (addr >> 32) & 0xffffffff;
    m_entries[i].selector = selector;
    m_entries[i].flags = flags;
    m_entries[i].ist = 0;
    m_entries[i].reserved = 0;
}

void
IDT::dump(unsigned index) const
{
    unsigned start = 0;
    unsigned count = (m_ptr.limit + 1) / sizeof(descriptor);
    if (index != -1) {
        start = index;
        count = 1;
        log::info(logs::boot, "IDT FOR INDEX %02x", index);
    } else {
        log::info(logs::boot, "Loaded IDT at: %lx size: %d bytes", m_ptr.base, m_ptr.limit+1);
    }

    const descriptor *idt =
        reinterpret_cast<const descriptor *>(m_ptr.base);

    for (int i = start; i < start+count; ++i) {
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
