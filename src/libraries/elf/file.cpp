#include <elf/file.h>
#include <elf/headers.h>

static const uint32_t expected_magic = 0x464c457f;  // "\x7f" "ELF"

namespace elf {

inline const file_header * fh(util::const_buffer data) { return reinterpret_cast<const file_header*>(data.pointer); }

template <typename T>
const T *convert(util::const_buffer data, size_t offset) {
    return reinterpret_cast<const T*>(util::offset_pointer(data.pointer, offset));
}

file::file(util::const_buffer data) :
    m_segments(convert<segment_header>(data, fh(data)->ph_offset), fh(data)->ph_entsize, fh(data)->ph_num),
    m_sections(convert<section_header>(data, fh(data)->sh_offset), fh(data)->sh_entsize, fh(data)->sh_num),
    m_data(data)
{
}

bool
file::valid() const
{
    if (m_data.count < sizeof(file_header))
        return false;

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

uintptr_t
file::entrypoint() const
{
    return static_cast<uintptr_t>(header()->entrypoint);
}


} // namespace elf
