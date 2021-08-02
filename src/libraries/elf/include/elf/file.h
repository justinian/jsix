#pragma once
#include <stddef.h>
#include <stdint.h>

#include "pointer_manipulation.h"

namespace elf {

struct file_header;
struct program_header;
struct section_header;

template <typename T>
class subheaders
{
public:
    using iterator = const_offset_iterator<T>;

    subheaders(const T *start, size_t size, unsigned count) :
        m_start(start), m_size(size), m_count(count) {}

    inline size_t size() const { return m_size; }
    inline unsigned count() const { return m_count; }

    inline const T & operator [] (int i) const { return *offset_ptr<T>(m_start, m_size*i); }
    inline const iterator begin() const { return iterator(m_start, m_size); }
    inline const iterator end() const { return offset_ptr<T>(m_start, m_size*m_count); }

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
    /// \arg data  The ELF data to read
    /// \arg size  Size of the ELF data, in bytes
    file(const void *data, size_t size);

    /// Check the validity of the ELF data
    /// \returns  true for valid ELF data
    bool valid() const;

    /// Get the entrypoint address of the program image
    /// \returns  A pointer to the entrypoint of the program
    uintptr_t entrypoint() const;

    /// Get the base address of the program in memory
    inline uintptr_t base() const {
        return reinterpret_cast<uintptr_t>(m_data);
    }

    /// Get the ELF program headers
    inline const subheaders<program_header> & programs() const { return m_programs; }

    /// Get the ELF section headers
    inline const subheaders<section_header> & sections() const { return m_sections; }

    inline const file_header * header() const {
        return reinterpret_cast<const file_header *>(m_data);
    }

private:
    subheaders<program_header> m_programs;
    subheaders<section_header> m_sections;

    const void *m_data;
    size_t m_size;
};

}
