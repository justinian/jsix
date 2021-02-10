#pragma once
/// \file tss.h
/// Definitions relating to the TSS
#include <stdint.h>

/// The 64bit TSS table
class TSS
{
public:
	TSS();

	/// Get the currently running CPU's TSS.
	static TSS & current();

	/// Ring stack accessor. Returns a mutable reference.
	/// \arg ring  Which ring (0-3) to get the stack for
	/// \returns   A mutable reference to the stack pointer
	uintptr_t & ring_stack(unsigned ring);

	/// IST stack accessor. Returns a mutable reference.
	/// \arg ist   Which IST entry (1-7) to get the stack for
	/// \returns   A mutable reference to the stack pointer
	uintptr_t & ist_stack(unsigned ist);

	/// Allocate new stacks for the given IST entries.
	/// \arg ist_entries  A bitmap of used IST entries
	void create_ist_stacks(uint8_t ist_entries);

private:
	uint32_t m_reserved0;

	uintptr_t m_rsp[3]; // stack pointers for CPL 0-2
	uintptr_t m_ist[8]; // ist[0] is reserved

	uint64_t m_reserved1;
	uint16_t m_reserved2;
	uint16_t m_iomap_offset;
} __attribute__ ((packed));

