#pragma once
/// \file idt.h
/// Definitions relating to a CPU's IDT table
#include <stdint.h>

class IDT
{
public:
	static constexpr unsigned count = 256;

	IDT();

	/// Install this IDT to the current CPU
	void install() const;

	/// Add the IST entries listed in the ISR table into the IDT.
	/// This can't be done until after memory is set up so the
	/// stacks can be created.
	void add_ist_entries();

	/// Get the IST entry used by an entry.
	/// \arg i   Which IDT entry to look in
	/// \returns The IST index used by entry i, or 0 for none
	inline uint8_t get_ist(unsigned i) const {
		if (i >= count) return 0;
		return m_entries[i].ist;
	}

	/// Get the IST entries that are used by this table, as a bitmap
	uint8_t used_ist_entries() const;

	/// Dump debug information about the IDT to the console.
	/// \arg index  Which entry to print, or -1 for all entries
	void dump(unsigned index = -1) const;

private:
	void set(uint8_t i, void (*handler)(), uint16_t selector, uint8_t flags);
	void set_ist(uint8_t i, uint8_t ist);

	struct descriptor
	{
		uint16_t base_low;
		uint16_t selector;
		uint8_t ist;
		uint8_t flags;
		uint16_t base_mid;
		uint32_t base_high;
		uint32_t reserved;		// must be zero
	} __attribute__ ((packed, aligned(16)));

	struct ptr
	{
		uint16_t limit;
		descriptor *base;
	} __attribute__ ((packed, aligned(4)));

	descriptor m_entries[256];
	ptr m_ptr;
};

extern IDT &g_idt;
