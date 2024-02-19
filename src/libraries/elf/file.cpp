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
    m_segments {convert<segment_header>(data, fh(data)->ph_offset), fh(data)->ph_entsize, fh(data)->ph_num},
    m_sections {convert<section_header>(data, fh(data)->sh_offset), fh(data)->sh_entsize, fh(data)->sh_num},
    m_data {data}
{

}

bool
file::valid(elf::filetype type) const
{
    if (m_data.count < sizeof(file_header))
        return false;

    const file_header *fheader = header();

    bool abi_valid =
        fheader->magic == expected_magic &&
        fheader->word_size == wordsize::bits64 &&
        fheader->endianness == encoding::lsb &&
        fheader->os_abi == osabi::sysV &&
        fheader->machine_type == machine::x64 &&
        fheader->ident_version == 1 &&
        fheader->version == 1;

    if (!abi_valid)
        return false;

    if (type != filetype::none && fheader->file_type != type)
        return false;

    return true;
}

uintptr_t
file::entrypoint() const
{
    return static_cast<uintptr_t>(header()->entrypoint);
}



const section_header *
file::get_section_by_name(const char *name)
{
    if (!name)
        return nullptr;

    const section_header &shstrtab = m_sections[header()->sh_str_idx];
    util::counted<const char> strings = {
        reinterpret_cast<const char*>(m_data.pointer) + shstrtab.offset,
        shstrtab.size,
    };

    for (auto &sec : m_sections) {
        const char *section_name = &strings[sec.name_offset];
        size_t i = 0;

        // Can't depend on libc, do our own basic strcmp here
        while (name[i] && section_name[i] && name[i] == section_name[i]) ++i;
        if (name[i] == section_name[i])
            return &sec;
    }
    return nullptr;
}

} // namespace elf
