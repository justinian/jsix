#pragma once
/// \file msr.h
/// Routines and definitions for dealing with Model-Specific Registers

#include <stdint.h>

enum class msr : uint32_t
{
	ia32_efer			= 0xc0000080,
	ia32_star			= 0xc0000081,
	ia32_lstar			= 0xc0000082,
	ia32_fmask			= 0xc0000084,

	ia32_gs_base		= 0xc0000101,
	ia32_kernel_gs_base = 0xc0000102
};

/// Read the value of a MSR
/// \arg addr   The MSR address
/// \returns    The current value of the MSR
uint64_t rdmsr(msr addr);

/// Write to a MSR
/// \arg addr   The MSR address
/// \arg value  The value to write
void wrmsr(msr addr, uint64_t value);

