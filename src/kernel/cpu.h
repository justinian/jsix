#pragma once


class cpu_id
{
public:
	struct regs {
		uint32_t eax, ebx, ecx, edx;

		regs() : eax(0), ebx(0), ecx(0), edx(0) {}
		regs(uint32_t a, uint32_t b, uint32_t c, uint32_t d) : eax(a), ebx(b), ecx(c), edx(d) {}
		regs(const regs &r) : eax(r.eax), ebx(r.ebx), ecx(r.ecx), edx(r.edx) {}

		bool eax_bit(unsigned bit) { return (eax >> bit) & 0x1; }
		bool ebx_bit(unsigned bit) { return (ebx >> bit) & 0x1; }
		bool ecx_bit(unsigned bit) { return (ecx >> bit) & 0x1; }
		bool edx_bit(unsigned bit) { return (edx >> bit) & 0x1; }
	};

	cpu_id();

	regs get(uint32_t leaf, uint32_t sub = 0) const;

	inline const char * vendor_id() const	{ return m_vendor_id; }
	inline uint8_t cpu_type() const			{ return m_cpu_type; }
	inline uint8_t stepping() const			{ return m_stepping; }
	inline uint16_t family() const			{ return m_family; }
	inline uint16_t model() const			{ return m_model; }

private:
	void read();

	uint32_t m_high_leaf;
	char m_vendor_id[13];

	uint8_t m_cpu_type;
	uint8_t m_stepping;
	uint16_t m_family;
	uint16_t m_model;
};
