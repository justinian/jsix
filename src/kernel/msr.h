#pragma once
/// \file msr.h
/// Routines and definitions for dealing with Model-Specific Registers

#include <stdint.h>

enum class msr : uint32_t
{
	ia32_mtrrcap           = 0x000000fe,
	ia32_mtrrdeftype       = 0x000002ff,

	ia32_mtrrphysbase      = 0x00000200,
	ia32_mtrrphysmask      = 0x00000201,

	ia32_mtrrfix64k_00000  = 0x00000250,

	ia32_mtrrfix16k_80000  = 0x00000258,
	ia32_mtrrfix16k_a0000  = 0x00000259,

	ia32_mtrrfix4k_c0000   = 0x00000268,
	ia32_mtrrfix4k_c8000   = 0x00000269,
	ia32_mtrrfix4k_d0000   = 0x0000026A,
	ia32_mtrrfix4k_d8000   = 0x0000026B,
	ia32_mtrrfix4k_e0000   = 0x0000026C,
	ia32_mtrrfix4k_e8000   = 0x0000026D,
	ia32_mtrrfix4k_f0000   = 0x0000026E,
	ia32_mtrrfix4k_f8000   = 0x0000026F,

	ia32_pat               = 0x00000277,
	ia32_efer              = 0xc0000080,
	ia32_star              = 0xc0000081,
	ia32_lstar             = 0xc0000082,
	ia32_fmask             = 0xc0000084,

	ia32_gs_base           = 0xc0000101,
	ia32_kernel_gs_base    = 0xc0000102
};

/// Find the msr for MTRR physical base or mask
msr find_mtrr(msr type, unsigned index);

/// Read the value of a MSR
/// \arg addr  The MSR address
/// \returns   The current value of the MSR
uint64_t rdmsr(msr addr);

/// Write to a MSR
/// \arg addr  The MSR address
/// \arg value The value to write
void wrmsr(msr addr, uint64_t value);

