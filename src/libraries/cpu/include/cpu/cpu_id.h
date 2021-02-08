#pragma once
/// \file cpu_id.h Definition of required cpu features for jsix

#include <stdint.h>

namespace cpu {

/// Enum of the cpu features jsix cares about
enum class feature {
#define CPU_FEATURE_REQ(name, ...)  name,
#define CPU_FEATURE_OPT(name, ...)  name,
#include "cpu/features.inc"
#undef CPU_FEATURE_OPT
#undef CPU_FEATURE_REQ
	max
};

class cpu_id
{
public:
	static constexpr uint32_t cpuid_extended = 0x80000000;

	/// CPUID result register values
	struct regs {
		union {
			uint32_t reg[4];
			uint32_t eax, ebx, ecx, edx;
		};

		/// Return true if bit |bit| of EAX is set
		bool eax_bit(unsigned bit) { return (eax >> bit) & 0x1; }

		/// Return true if bit |bit| of EBX is set
		bool ebx_bit(unsigned bit) { return (ebx >> bit) & 0x1; }

		/// Return true if bit |bit| of ECX is set
		bool ecx_bit(unsigned bit) { return (ecx >> bit) & 0x1; }

		/// Return true if bit |bit| of EDX is set
		bool edx_bit(unsigned bit) { return (edx >> bit) & 0x1; }
	};

	cpu_id();

	/// The the result of a given CPUID leaf/subleaf
	/// \arg leaf     The leaf selector (initial EAX)
	/// \arg subleaf  The subleaf selector (initial ECX)
	/// \returns      A |regs| struct of the values retuned
	regs get(uint32_t leaf, uint32_t sub = 0) const;

	/// Get the local APIC ID of the current CPU
	uint8_t local_apic_id() const;

	/// Get the name of the cpu vendor (eg, "GenuineIntel")
	inline const char * vendor_id() const	{ return m_vendor_id; }

	/// Get the brand name of this processor model
	inline const char * brand_name() const	{ return m_brand_name; }

	/// Get the highest basic CPUID leaf supported
	inline uint32_t highest_basic() const	{ return m_high_basic; }

	/// Get the highest extended CPUID leaf supported
	inline uint32_t highest_ext() const		{ return m_high_ext; }

	/// Get which required options are missing as flags
	inline uint64_t missing() const         { return m_missing; }

	/// Validate the CPU supports the necessary options for jsix
	inline bool supported() const           { return m_missing; }

	/// Return true if the CPU claims to support the given feature
	bool has_feature(feature feat);

private:
	uint32_t m_high_basic;
	uint32_t m_high_ext;
	uint64_t m_features;
	uint64_t m_missing;
	char m_vendor_id[13];
	char m_brand_name[48];
};

}
