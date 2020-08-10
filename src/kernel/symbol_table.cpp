#include "kutil/assert.h"
#include "kutil/memory.h"

#include "log.h"
#include "symbol_table.h"

symbol_table * symbol_table::s_instance = nullptr;

symbol_table::symbol_table(const void *data, size_t size)
{
	m_data = kutil::kalloc(size);
	kassert(m_data, "Failed to allocate for symbol table");
	kutil::memcpy(m_data, data, size);

	uint64_t *values = reinterpret_cast<uint64_t*>(m_data);
	m_entries = *values++;
	m_index = reinterpret_cast<entry*>(values);

	for (size_t i = 0; i < m_entries; ++i) {
		uint64_t offset = reinterpret_cast<uint64_t>(m_index[i].name);
		m_index[i].name = reinterpret_cast<char*>(m_data) + offset;
	}

	if (!s_instance)
		s_instance = this;

	log::info(logs::boot, "Loaded %d symbol table entries at %llx", m_entries, m_data);
}

symbol_table::~symbol_table()
{
	kutil::kfree(m_data);
}

const symbol_table::entry *
symbol_table::find_symbol(uintptr_t addr) const
{
	if (!m_entries)
		return nullptr;

	// TODO: binary search
	for (size_t i = 0; i < m_entries; ++i) {
		if (m_index[i].address > addr) {
			return i ? &m_index[i - 1] : nullptr;
		}
	}

	return &m_index[m_entries - 1];
}
