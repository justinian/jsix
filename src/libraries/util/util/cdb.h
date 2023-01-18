/// \file cdb.h
/// Helper functions and types for working with djb's constant database archives
#pragma once

#include <stdint.h>
#include <util/counted.h>

namespace util {

class cdb
{
public:
    cdb(buffer data);

    /// Retrieve a value from the database for the given key.
    /// \arg key  A null-terminated string key
    /// \returns  A const util::buffer pointing to the data in memory.
    ///           The buffer will be {0, 0} if the key is not found.
    const buffer retrieve(const char *key) const;

    /// Retrieve a value from the database for the given key.
    /// \arg key  Pointer to a key as an array of bytes
    /// \arg len  Length of the key
    /// \returns  A const util::buffer pointing to the data in memory.
    ///           The buffer will be {0, 0} if the key is not found.
    const buffer retrieve(const uint8_t *key, uint32_t len) const;

private:
    template <typename T>
    T const * at(uint32_t offset) const {
        return reinterpret_cast<T const*>(util::offset_pointer<const void>(m_data.pointer, offset));
    }

    buffer m_data;
};

} // namespace
