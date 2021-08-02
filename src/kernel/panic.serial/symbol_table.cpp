#include "pointer_manipulation.h"
#include "symbol_table.h"

namespace panic {

symbol_table::symbol_table(const void *data) :
    m_data(data)
{
    const size_t *count = reinterpret_cast<const size_t*>(data);
    m_entries = {
        .pointer = reinterpret_cast<entry const*>(count+1),
        .count = *count,
    };
}

const char *
symbol_table::find_symbol(uintptr_t addr) const
{
    // TODO: binary search
    for (auto &e : m_entries) {
        if (addr >= e.address && addr < e.address + e.size)
            return reinterpret_cast<const char*>(m_data) + e.name;
    }

    return nullptr;
}

} // namespace panic
