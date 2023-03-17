#include <stdint.h>
#include "cpu/cpu_id.h"

namespace cpu {

inline static void
__cpuid(
    uint32_t leaf,
    uint32_t subleaf,
    uint32_t *eax,
    uint32_t *ebx = nullptr,
    uint32_t *ecx = nullptr,
    uint32_t *edx = nullptr)
{
    uint32_t a, b, c, d;
    __asm__ __volatile__ ( "cpuid"
            : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
            : "a"(leaf), "c"(subleaf)
            );
    if (eax) *eax = a;
    if (ebx) *ebx = b;
    if (ecx) *ecx = c;
    if (edx) *edx = d;
}

cpu_id::cpu_id()
{
    __cpuid(0, 0,
        &m_high_basic,
        reinterpret_cast<uint32_t *>(&m_vendor_id[0]),
        reinterpret_cast<uint32_t *>(&m_vendor_id[8]),
        reinterpret_cast<uint32_t *>(&m_vendor_id[4]));

    __cpuid(cpuid_extended, 0, &m_high_ext);
}

features
cpu_id::features() const
{
    ::cpu::features feats;
    uint32_t leaf = -1u;
    uint32_t sub = -1u;
    regs r;

#define CPU_FEATURE_OPT(name, feat_leaf, feat_sub, regname, bit) \
    if (leaf != feat_leaf || sub != feat_sub) { \
        leaf = feat_leaf; sub = feat_sub; r = get(leaf, sub); \
    } \
    if (r.regname & (1ull << bit)) \
        feats.set(feature::name);

#define CPU_FEATURE_REQ(name, feat_leaf, feat_sub, regname, bit) \
    CPU_FEATURE_OPT(name, feat_leaf, feat_sub, regname, bit);

#include "cpu/features.inc"
#undef CPU_FEATURE_OPT
#undef CPU_FEATURE_REQ

    return feats;
}

cpu_id::regs
cpu_id::get(uint32_t leaf, uint32_t sub) const
{
    regs ret {0, 0, 0, 0};

    if ((leaf & cpuid_extended) == 0 && leaf > m_high_basic) return ret;
    if ((leaf & cpuid_extended) != 0 && leaf > m_high_ext) return ret;

    __cpuid(leaf, sub, &ret.eax, &ret.ebx, &ret.ecx, &ret.edx);
    return ret;
}

uint8_t
cpu_id::local_apic_id() const
{
    uint32_t eax_unused;
    uint32_t ebx;
    __cpuid(1, 0, &eax_unused, &ebx);
    return static_cast<uint8_t>(ebx >> 24);
}

bool
cpu_id::brand_name(char *buffer) const
{
    if (m_high_ext < cpuid_extended + 4)
        return false;

    __cpuid(cpuid_extended + 2, 0,
        reinterpret_cast<uint32_t *>(&buffer[0]),
        reinterpret_cast<uint32_t *>(&buffer[4]),
        reinterpret_cast<uint32_t *>(&buffer[8]),
        reinterpret_cast<uint32_t *>(&buffer[12]));
    __cpuid(cpuid_extended + 3, 0,
        reinterpret_cast<uint32_t *>(&buffer[16]),
        reinterpret_cast<uint32_t *>(&buffer[20]),
        reinterpret_cast<uint32_t *>(&buffer[24]),
        reinterpret_cast<uint32_t *>(&buffer[28]));
    __cpuid(cpuid_extended + 4, 0,
        reinterpret_cast<uint32_t *>(&buffer[32]),
        reinterpret_cast<uint32_t *>(&buffer[36]),
        reinterpret_cast<uint32_t *>(&buffer[40]),
        reinterpret_cast<uint32_t *>(&buffer[44]));

    return true;
}

}
