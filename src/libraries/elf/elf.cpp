#include "elf/elf.h"
#include "elf/headers.h"

static const uint32_t expected_magic = 0x464c457f;  // "\x7f" "ELF"

namespace elf {

elf::elf(const void *data, size_t size) :
	m_data(data),
	m_size(size)
{
}

bool
elf::valid() const
{
	const file_header *fheader = header();

	return
		fheader->magic == expected_magic &&
		fheader->word_size == wordsize::bits64 &&
		fheader->endianness == encoding::lsb &&
		fheader->os_abi == osabi::sysV &&
		fheader->file_type == filetype::executable &&
		fheader->machine_type == machine::x64 &&
		fheader->ident_version == 1 &&
		fheader->version == 1;
}

const program_header *
elf::program(unsigned index) const
{
	const file_header *fheader = header();
	uint64_t off = fheader->ph_offset + (index * fheader->ph_entsize);
	const void *pheader = kutil::offset_pointer(m_data, off);
	return reinterpret_cast<const program_header *>(pheader);
}

const section_header *
elf::section(unsigned index) const
{
	const file_header *fheader = header();
	uint64_t off = fheader->sh_offset + (index * fheader->sh_entsize);
	const void *sheader = kutil::offset_pointer(m_data, off);
	return reinterpret_cast<const section_header *>(sheader);
}


} // namespace elf
