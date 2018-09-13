#include <stdint.h>
#include "kutil/memory.h"
#include "cpu.h"
#include "log.h"

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
		&m_high_leaf,
		reinterpret_cast<uint32_t *>(&m_vendor_id[0]),
		reinterpret_cast<uint32_t *>(&m_vendor_id[8]),
		reinterpret_cast<uint32_t *>(&m_vendor_id[4]));

	uint32_t eax = 0;
	__cpuid(0, 1, &eax);

	m_stepping = eax & 0xf;
	m_model = (eax >> 4) & 0xf;
	m_family = (eax >> 8) & 0xf;

	m_cpu_type = (eax >> 12) & 0x3;

	uint32_t ext_model = (eax >> 16) & 0xf;
	uint32_t ext_family = (eax >> 20) & 0xff;

	if (m_family == 0x6 || m_family == 0xf)
		m_model = (ext_model << 4) + m_model;

	if (m_family == 0xf)
		m_family += ext_family;

}


cpu_id::regs
cpu_id::get(uint32_t leaf, uint32_t sub) const
{
	if (leaf > m_high_leaf) return {};

	regs ret;
	__cpuid(leaf, sub, &ret.eax, &ret.ebx, &ret.ecx, &ret.edx);
	return ret;
}
