#include "initrd/headers.h"
#include "initrd/initrd.h"
#include "kutil/assert.h"
#include "kutil/enum_bitfields.h"

namespace initrd {

file::file(const file_header *header, const void *start) :
	m_header(header)
{
	m_data = kutil::offset_pointer(start, m_header->offset);
	auto *name = kutil::offset_pointer(start, m_header->name_offset);
	m_name = reinterpret_cast<const char *>(name);
}

const char * file::name() const { return m_name; }
const size_t file::size() const { return m_header->length; }
const void * file::data() const { return m_data; }

bool
file::executable() const {
	return bitfield_has(m_header->flags, file_flags::executable);
}

bool
file::symbols() const {
	return bitfield_has(m_header->flags, file_flags::symbols);
}


disk::disk(const void *start)
{
	auto *header = reinterpret_cast<const disk_header *>(start);
	size_t length = header->length;
	uint8_t sum = kutil::checksum(start, length);
	kassert(sum == 0, "initrd checksum failed");

	auto *files = reinterpret_cast<const file_header *>(header + 1);

	m_files.ensure_capacity(header->file_count);
	for (int i = 0; i < header->file_count; ++i) {
		m_files.emplace(&files[i], start);
	}
}

} // namespace initrd
