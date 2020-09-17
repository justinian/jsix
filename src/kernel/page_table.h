#pragma once
/// \file page_table.h
/// Helper structures for dealing with page tables.

#include <stdint.h>
#include "kernel_memory.h"

class page_manager;

/// Struct to allow easy accessing of a memory page being used as a page table.
struct page_table
{
	/// Enum representing the table levels in 4-level paging
	enum class level : unsigned { pml4, pdp, pd, pt, page };

	/// Helper for getting the next level value
	inline static level deeper(level l) {
		return static_cast<level>(static_cast<unsigned>(l) + 1);
	}

	static constexpr size_t entry_sizes[] = {
		0x8000000000, // PML4 entry: 512 GiB
		0x40000000,   //  PDP entry:   1 GiB
		0x200000,     //   PD entry:   2 MiB
		0x1000};      //   PT entry:   4 KiB

	/// Flag marking unused space as allowed for allocation
	static constexpr uint64_t flag_allowed = (1ull << 11);

	/// Iterator over page table entries.
	class iterator
	{
		/// The number of levels
		static constexpr unsigned D = 4;

	public:
		/// Constructor.
		/// \arg virt       Virtual address this iterator is starting at
		/// \arg pml4       Root of the page tables to iterate
		iterator(uintptr_t virt, page_table *pml4);

		/// Copy constructor.
		iterator(const iterator &o);

		/// Get the starting address of pages mapped by the current table
		/// of level l.
		uintptr_t start(level l) const;

		/// Get the ending address of pages mapped by the current table
		/// of level l.
		uintptr_t end(level l) const;

		/// Get the widest table type the current address is aligned to
		level align() const;

		/// Get the current virtual address
		inline uintptr_t vaddress() const { return start(level::pt); }

		/// Get the nth page table of the current address
		inline page_table *& table(level l) const { return m_table[unsigned(l)]; }

		/// Get the index in the nth page table of the current address
		inline uint16_t & index(level l) { return m_index[unsigned(l)]; }

		/// Get the index in the nth page table of the current address
		inline uint16_t index(level l) const { return m_index[unsigned(l)]; }

		/// Get the current table entry of the table at the given level.
		inline uint64_t entry(level l) const {
			for (unsigned i = 1; i <= unsigned(l); ++i)
				if (!check_table(level(i))) return 0;
			return table(l)->entries[index(l)];
		}

		/// Get a *non-const* reference to the current table entry of
		/// the table at the given level.
		inline uint64_t & entry(level l) {
			for (unsigned i = 1; i < unsigned(l); ++i) ensure_table(level(i));
			return table(l)->entries[index(l)];
		}

		/// Get the depth of tables that actually exist for the current address
		level depth() const;

		/// Increment iteration to the next entry aligned to the given level
		void next(level l);

		/// Check if allocation is allowed at the current location
		bool allowed() const;

		/// Mark allocation allowed at the given depth for the current location
		void allow(level at, bool allowed);

		/// Increment iteration to the next entry at the deepest level
		inline void increment() { next(level::page); }

		inline uint64_t & operator*() { return entry(level::pt); }
		inline iterator & operator++() { increment(); return *this; }
		inline iterator operator++(int) { iterator old {*this}; increment(); return old; }

		bool operator!=(const iterator &o) const;

		bool check_table(level l) const;
		void ensure_table(level l);

	private:
		// The table array is mutable because we might update it with existing
		// tables; a 'view switch'. therefore, be careful not to modify table
		// contents in const functions.
		mutable page_table *m_table[D];
		uint16_t m_index[D];
	};

	/// Get an entry in the page table as a page_table pointer
	/// \arg i     Index of the entry in this page table
	/// \arg flags [out] If set, this will receive the entry's flags
	/// \returns   The corresponding entry, offset into page-offset memory
	page_table * get(int i, uint16_t *flags = nullptr) const;

	/// Set an entry in the page table as a page_table pointer
	/// \arg i     Index of the entry in this page table
	/// \arg flags The flags for the entry
	void set(int i, page_table *p, uint16_t flags);

	/// Check if the given entry represents a large or huge page
	inline bool is_large_page(level l, int i) const {
		return (l == level::pdp || l == level::pd) && (entries[i] & 0x80) == 0x80;
	}

	/// Check if the given entry is marked as present in the table
	inline bool is_present(int i) const { return (entries[i] & 0x1) == 0x1; }

	/// Check if the given entry represents a page (of any size)
	inline bool is_page(level l, int i) const { return (l == level::pt) || is_large_page(l, i); }

	/// Print this table to the debug console.
	void dump(level lvl = level::pml4, bool recurse = true);

	uint64_t entries[memory::table_entries];
};


/// Helper struct for computing page table indices of a given address.
struct page_table_indices
{
	page_table_indices(uint64_t v = 0);

	uintptr_t addr() const;

	inline operator uintptr_t() const { return addr(); }

	/// Get the index for a given level of page table.
	uint64_t & operator[](int i) { return index[i]; }
	uint64_t operator[](int i) const { return index[i]; }
	uint64_t & operator[](page_table::level i) { return index[static_cast<unsigned>(i)]; }
	uint64_t operator[](page_table::level i) const { return index[static_cast<unsigned>(i)]; }
	uint64_t index[4]; ///< Indices for each level of tables.
};

bool operator==(const page_table_indices &l, const page_table_indices &r);

inline page_table::level operator+(page_table::level a, unsigned b) {
	return static_cast<page_table::level>(static_cast<unsigned>(a) + b);
}

inline page_table::level operator-(page_table::level a, unsigned b) {
	return static_cast<page_table::level>(static_cast<unsigned>(a) - b);
}

inline bool operator>(page_table::level a, page_table::level b) {
	return static_cast<unsigned>(a) > static_cast<unsigned>(b);
}

inline bool operator<(page_table::level a, page_table::level b) {
	return static_cast<unsigned>(a) < static_cast<unsigned>(b);
}

inline page_table::level& operator++(page_table::level& l) { l = l + 1; return l; }
inline page_table::level& operator--(page_table::level& l) { l = l - 1; return l; }
