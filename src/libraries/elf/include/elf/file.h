#pragma once
#include <stddef.h>
#include <stdint.h>

#include <elf/headers.h>
#include <util/counted.h>
#include <util/pointers.h>

namespace elf {

struct file_header;
struct segment_header;
struct section_header;

template <typename T>
class subheaders
{
public:
    using iterator = util::const_offset_iterator<T>;

    subheaders(const T *start, size_t size, unsigned count) :
        m_start(start), m_size(size), m_count(count) {}

    inline size_t size() const { return m_size; }
    inline unsigned count() const { return m_count; }

    inline const T & operator [] (int i) const { return *util::offset_pointer<const T>(m_start, m_size*i); }
    inline const iterator begin() const { return iterator(m_start, m_size); }
    inline const iterator end() const { return util::offset_pointer(m_start, m_size*m_count); }

private:
    const T *m_start;
    size_t m_size;
    unsigned m_count;
};

/// Represents a full ELF file's data
class file
{
public:
    /// Constructor: Create an elf object out of ELF data in memory
    /// \arg data  A const_buffer pointing at the ELF data
    file(util::const_buffer data);

    /// Check the validity of the ELF data
    /// \arg type  Expected file type of the data
    /// \returns   true for valid ELF data
    bool valid(elf::filetype type = elf::filetype::executable) const;

    /// Get the entrypoint address of the program image
    /// \returns  A pointer to the entrypoint of the program
    uintptr_t entrypoint() const;

    /// Get the base address of the program in memory
    inline uintptr_t base() const {
        return reinterpret_cast<uintptr_t>(m_data.pointer);
    }

    /// Get the ELF segment headers
    inline const subheaders<segment_header> & segments() const { return m_segments; }

    /// Get the ELF section headers
    inline const subheaders<section_header> & sections() const { return m_sections; }

    /// Get the ELF file header
    inline const file_header * header() const {
        return reinterpret_cast<const file_header *>(m_data.pointer);
    }

    const section_header * get_section_by_name(const char *);

    /// Get the ELF file type from the header
    inline filetype type() const { return header()->file_type; }

private:
    subheaders<segment_header> m_segments;
    subheaders<section_header> m_sections;

    util::const_buffer m_data;
};

}
