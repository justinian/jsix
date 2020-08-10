#pragma once

#include <stdint.h>

class symbol_table
{
public:
	struct entry
	{
		uintptr_t address;
		char *name;
	};

	static symbol_table * get() { return s_instance; }

	/// Constructor.
	/// \arg data  Pointer to the start of the symbol_table data.
	/// \arg size  Size of the data, in bytes
	symbol_table(const void *data, size_t size);

	~symbol_table();

	/// Find the closest symbol address before the given address
	/// \args addr  Address to search for
	/// \returns    Actual address of the symbol
	const entry * find_symbol(uintptr_t addr) const;

private:
	size_t m_entries;
	entry *m_index;
	void *m_data;

	static symbol_table *s_instance;
};
