#include "kutil/assert.h"
#include "apic.h"
#include "interrupts.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"


static uint32_t
apic_read(uint32_t volatile *apic, uint16_t offset)
{
	return *(apic + offset/sizeof(uint32_t));
}

static void
apic_write(uint32_t volatile *apic, uint16_t offset, uint32_t value)
{
	*(apic + offset/sizeof(uint32_t)) = value;
}

static uint32_t
ioapic_read(uint32_t volatile *base, uint8_t reg)
{
	*base = reg;
	return *(base + 4);
}

static void
ioapic_write(uint32_t volatile *base, uint8_t reg, uint32_t value)
{
	*base = reg;
	*(base + 4) = value;
}

apic::apic(uint32_t *base) :
	m_base(base)
{
	// Map 1MiB of space for the APIC registers and
	// MSI area
	page_manager::get()->map_offset_pointer(
			reinterpret_cast<void **>(&m_base),
			0x100000);
}


lapic::lapic(uint32_t *base, isr spurious) :
	apic(base)
{
	apic_write(m_base, 0xf0, static_cast<uint32_t>(spurious));
	log::info(logs::apic, "LAPIC created, base %lx", m_base);
}

void
lapic::calibrate_timer()
{
	interrupts_disable();

	log::info(logs::apic, "Calibrating APIC timer...");

	// Set up PIT sleep
	uint8_t command = 0x30; // channel 0, loybyte/highbyte, mode 0
	outb(0x43, command);

	const uint32_t initial = -1u;
	enable_timer_internal(isr::isrSpurious, 1, initial, false);

	const int iterations = 5;
	for (int i=0; i<iterations; ++i) {
		const uint16_t pit_33ms = 39375;
		uint16_t pit_count = pit_33ms;
		outb(0x40, pit_count & 0xff);
		io_wait();
		outb(0x40, (pit_count >> 8) & 0xff);


		while (pit_count <= pit_33ms) {
			outb(0x43, 0); // latch counter values
			pit_count =
				static_cast<uint16_t>(inb(0x40)) |
				static_cast<uint16_t>(inb(0x40)) << 8;
		}
	}

	uint32_t remain = stop_timer();
	uint32_t ticks_total = initial - remain;
	m_ticks_per_us = ticks_total / (iterations * 33000);
	log::info(logs::apic, "APIC timer ticks %d times per nanosecond.", m_ticks_per_us);

	interrupts_enable();
}

uint32_t
lapic::enable_timer_internal(isr vector, uint8_t divisor, uint32_t count, bool repeat)
{
	uint32_t divbits = 0;

	switch (divisor) {
	case   1: divbits = 0xb; break;
	case   2: divbits = 0x0; break;
	case   4: divbits = 0x1; break;
	case   8: divbits = 0x2; break;
	case  16: divbits = 0x3; break;
	case  32: divbits = 0x8; break;
	case  64: divbits = 0x9; break;
	case 128: divbits = 0xa; break;
	default:
		kassert(0, "Invalid divisor passed to lapic::enable_timer");
	}

	uint32_t lvte = static_cast<uint8_t>(vector);
	if (repeat)
		lvte |= 0x20000;

	log::debug(logs::apic, "Enabling APIC timer with isr %d.", vector);
	apic_write(m_base, 0x320, lvte);
	apic_write(m_base, 0x3e0, divbits);

	reset_timer(count);
	return count;
}

uint32_t
lapic::enable_timer(isr vector, uint64_t interval, bool repeat)
{
	uint64_t ticks = interval * m_ticks_per_us;

	int divisor = 1;
	while (ticks > -1u) {
		ticks /= 2;
		divisor *= 2;
	}

	log::debug(logs::apic, "Enabling APIC timer count %ld, divisor %d.", ticks, divisor);
	return enable_timer_internal(vector, divisor, static_cast<uint32_t>(ticks), repeat);
}

uint32_t
lapic::reset_timer(uint32_t count)
{
	uint32_t remaining = apic_read(m_base, 0x390);
	apic_write(m_base, 0x380, count);
	return remaining;
}

void
lapic::enable_lint(uint8_t num, isr vector, bool nmi, uint16_t flags)
{
	kassert(num == 0 || num == 1, "Invalid LINT passed to lapic::enable_lint.");

	uint16_t off = num ? 0x360 : 0x350;
	uint32_t lvte = static_cast<uint8_t>(vector);

	uint16_t polarity = flags & 0x3;
	if (polarity == 3)
		lvte |= (1 << 13);

	uint16_t trigger = (flags >> 2) & 0x3;
	if (trigger == 3)
		lvte |= (1 << 15);

	apic_write(m_base, off, lvte);
	log::debug(logs::apic, "APIC LINT%d enabled as %s %d %s-triggered, active %s.",
			num, nmi ? "NMI" : "ISR", vector,
			polarity == 3 ? "level" : "edge",
			trigger == 3 ? "low" : "high");
}

void
lapic::enable()
{
	apic_write(m_base, 0xf0,
			apic_read(m_base, 0xf0) | 0x100);
	log::debug(logs::apic, "LAPIC enabled!");
}

void
lapic::disable()
{
	apic_write(m_base, 0xf0,
			apic_read(m_base, 0xf0) & ~0x100);
	log::debug(logs::apic, "LAPIC disabled.");
}


ioapic::ioapic(uint32_t *base, uint32_t base_gsi) :
	apic(base),
	m_base_gsi(base_gsi)
{
	uint32_t id = ioapic_read(m_base, 0);
	uint32_t version = ioapic_read(m_base, 1);

	m_id = (id >> 24) & 0xff;
	m_version = version & 0xff;
	m_num_gsi = (version >> 16) & 0xff;
	log::debug(logs::apic, "IOAPIC %d loaded, version %d, GSIs %d-%d",
			m_id, m_version, base_gsi, base_gsi + (m_num_gsi - 1));

	for (uint8_t i = 0; i < m_num_gsi; ++i) {
		uint16_t flags = (i < 0x10) ? 0 : 0xf;
		isr vector = isr::irq00 + i;
		redirect(i, vector, flags, true);
	}
}

void
ioapic::redirect(uint8_t irq, isr vector, uint16_t flags, bool masked)
{
	uint64_t entry = static_cast<uint64_t>(vector);

	uint16_t polarity = flags & 0x3;
	if (polarity == 3)
		entry |= (1 << 13);

	uint16_t trigger = (flags >> 2) & 0x3;
	if (trigger == 3)
		entry |= (1 << 15);

	if (masked)
		entry |= (1 << 16);

	ioapic_write(m_base, (2 * irq) + 0x10, static_cast<uint32_t>(entry & 0xffffffff));
	ioapic_write(m_base, (2 * irq) + 0x11, static_cast<uint32_t>(entry >> 32));
}

void
ioapic::mask(uint8_t irq, bool masked)
{
	uint32_t entry = ioapic_read(m_base, (2 * irq) + 0x10);
	if (masked)
		entry |= (1 << 16);
	else
		entry &= ~(1 << 16);

	ioapic_write(m_base, (2 * irq) + 0x10, entry);
}

void
ioapic::dump_redirs() const
{
	log::debug(logs::apic, "IOAPIC %d redirections:", m_id);

	for (uint8_t i = 0; i < m_num_gsi; ++i) {
		uint64_t low = ioapic_read(m_base, 0x10 + (2 *i));
		uint64_t high = ioapic_read(m_base, 0x10 + (2 *i));
		uint64_t redir = low | (high << 32);
		if (redir == 0) continue;

		uint8_t vector = redir & 0xff;
		uint8_t mode = (redir >> 8) & 0x7;
		uint8_t dest_mode = (redir >> 11) & 0x1;
		uint8_t polarity = (redir >> 13) & 0x1;
		uint8_t trigger = (redir >> 15) & 0x1;
		uint8_t mask = (redir >> 16) & 0x1;
		uint8_t dest = (redir >> 56) & 0xff;

		log::debug(logs::apic, "  %2d: vec %3d %s active, %s-triggered %s dest %d: %x",
				m_base_gsi + i, vector,
				polarity ? "low" : "high",
				trigger ? "level" : "edge",
				mask ? "masked" : "",
				dest_mode,
				dest);
	}
}
