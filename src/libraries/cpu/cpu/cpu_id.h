#pragma once
/// \file cpu_id.h Definition of required cpu features for jsix

#include <stdint.h>
#include <util/bitset.h>

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

using features = util::bitset<(unsigned)feature::max>;

class cpu_id
{
public:
    static constexpr uint32_t cpuid_extended = 0x80000000;

    /// CPUID result register values
    struct regs {
        union {
            uint32_t reg[4];
            struct {
                uint32_t eax, ebx, ecx, edx;
            };
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

    /// Check which of the cpu::feature flags this CPU supports.
    /// \returns  A util::bitset mapping to cpu::feature values
    features features() const;

    /// The the result of a given CPUID leaf/subleaf
    /// \arg leaf     The leaf selector (initial EAX)
    /// \arg subleaf  The subleaf selector (initial ECX)
    /// \returns      A |regs| struct of the values retuned
    regs get(uint32_t leaf, uint32_t sub = 0) const;

    /// Get the local APIC ID of the current CPU
    uint8_t local_apic_id() const;

    /// Get the name of the cpu vendor (eg, "GenuineIntel")
    inline const char * vendor_id() const   { return m_vendor_id; }

    /// Put the brand name of this processor model into the
    /// provided buffer.
    /// \arg buffer  Pointer to a buffer at least 48 bytes long
    /// \returns     True if the brand name was obtained
    bool brand_name(char *buffer) const;

    /// Get the highest basic CPUID leaf supported
    inline uint32_t highest_basic() const   { return m_high_basic; }

    /// Get the highest extended CPUID leaf supported
    inline uint32_t highest_ext() const     { return m_high_ext; }

private:
    uint32_t m_high_basic;
    uint32_t m_high_ext;
    char m_vendor_id[13];
};

}
