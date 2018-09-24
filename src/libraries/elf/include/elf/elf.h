#pragma once
#include <stddef.h>
#include <stdint.h>
#include "elf/headers.h"

namespace elf {

class elf
{
public:
	/// Constructor: Create an elf object out of ELF data in memory
	/// \arg data  The ELF data to read
	/// \arg size  Size of the ELF data, in bytes
	elf(const void *data, size_t size);

	/// Check the validity of the ELF data
	/// \returns  true for valid ELF data
	bool valid() const;

	/// Get the entrypoint address of the program image
	/// \returns  A pointer to the entrypoint of the program
	inline uintptr_t entrypoint() const
	{
		return static_cast<uintptr_t>(header()->entrypoint);
	}

	/// Get the number of program sections in the image
	/// \returns The number of program section entries
	inline unsigned program_count() const
	{
		return header()->ph_num;
	}

	/// Get a program header
	/// \arg index  The index number of the program header
	/// \returns    A pointer to the program header data
	const program_header * program(unsigned index) const;

	/// Get the number of data sections in the image
	/// \returns The number of section entries
	inline unsigned section_count() const
	{
		return header()->sh_num;
	}

	/// Get a section header
	/// \arg index  The index number of the section header
	/// \returns    A pointer to the section header data
	const section_header * section(unsigned index) const;

private:
	inline const file_header *header() const
	{
		return reinterpret_cast<const file_header *>(m_data);
	}

	const void *m_data;
	size_t m_size;
};

}
