#include <stdint.h>
#include "cpu/cpu.h"

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

cpu_id::cpu_id() :
	m_features {0},
	m_missing {0}
{
	__cpuid(0, 0,
		&m_high_basic,
		reinterpret_cast<uint32_t *>(&m_vendor_id[0]),
		reinterpret_cast<uint32_t *>(&m_vendor_id[8]),
		reinterpret_cast<uint32_t *>(&m_vendor_id[4]));

	__cpuid(cpuid_extended, 0, &m_high_ext);

	if (m_high_ext >= cpuid_extended + 4) {
		__cpuid(cpuid_extended + 2, 0,
			reinterpret_cast<uint32_t *>(&m_brand_name[0]),
			reinterpret_cast<uint32_t *>(&m_brand_name[4]),
			reinterpret_cast<uint32_t *>(&m_brand_name[8]),
			reinterpret_cast<uint32_t *>(&m_brand_name[12]));
		__cpuid(cpuid_extended + 3, 0,
			reinterpret_cast<uint32_t *>(&m_brand_name[16]),
			reinterpret_cast<uint32_t *>(&m_brand_name[20]),
			reinterpret_cast<uint32_t *>(&m_brand_name[24]),
			reinterpret_cast<uint32_t *>(&m_brand_name[28]));
		__cpuid(cpuid_extended + 4, 0,
			reinterpret_cast<uint32_t *>(&m_brand_name[32]),
			reinterpret_cast<uint32_t *>(&m_brand_name[36]),
			reinterpret_cast<uint32_t *>(&m_brand_name[40]),
			reinterpret_cast<uint32_t *>(&m_brand_name[44]));
	} else {
		m_brand_name[0] = 0;
	}

	uint32_t leaf = -1u;
	uint32_t sub = -1u;
	regs r;
#define CPU_FEATURE_OPT(name, feat_leaf, feat_sub, regname, bit) \
	if (leaf != feat_leaf || sub != feat_sub) { \
		leaf = feat_leaf; sub = feat_sub; r = get(leaf, sub); \
	} \
	if (r.regname & (1ull << bit)) \
		m_features |= (1ull << static_cast<uint64_t>(feature::name)); \

#define CPU_FEATURE_REQ(name, feat_leaf, feat_sub, regname, bit) \
	CPU_FEATURE_OPT(name, feat_leaf, feat_sub, regname, bit); \
	if ((r.regname & (1ull << bit)) == 0) { \
		m_missing |= (1ull << static_cast<uint64_t>(feature::name)); \
	}

#include "cpu/features.inc"
#undef CPU_FEATURE_OPT
#undef CPU_FEATURE_REQ
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

bool
cpu_id::has_feature(feature feat)
{
	return (m_features & (1 << static_cast<uint64_t>(feat))) != 0;
}

}
