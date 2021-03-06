#include "kutil/assert.h"
#include "kutil/memory.h"
#include "console.h"
#include "frame_allocator.h"
#include "kernel_memory.h"
#include "page_table.h"

using memory::page_offset;
using level = page_table::level;

free_page_header * page_table::s_page_cache = nullptr;
size_t page_table::s_cache_count = 0;
constexpr size_t page_table::entry_sizes[4];


constexpr page_table::flag table_flags =
	page_table::flag::present |
	page_table::flag::write;


page_table::iterator::iterator(uintptr_t virt, page_table *pml4) :
	m_table {pml4, 0, 0, 0}
{
	for (unsigned i = 0; i < D; ++i)
		m_index[i] = static_cast<uint16_t>((virt >> (12 + 9*(3-i))) & 0x1ff);
}

page_table::iterator::iterator(const page_table::iterator &o)
{
	kutil::memcpy(&m_table, &o.m_table, sizeof(m_table));
	kutil::memcpy(&m_index, &o.m_index, sizeof(m_index));
}

inline static level to_lv(unsigned i) { return static_cast<level>(i); }
inline static unsigned to_un(level i) { return static_cast<unsigned>(i); }

uintptr_t
page_table::iterator::start(level l) const
{
	uintptr_t address = 0;

	for (level i = level::pml4; i <= l; ++i)
		address |= static_cast<uintptr_t>(index(i)) << (12 + 9*(3-unsigned(i)));

	// canonicalize the address
	if (address & (1ull<<47))
		address |= (0xffffull<<48);

	return address;
}

uintptr_t
page_table::iterator::end(level l) const
{
	kassert(l != level::pml4, "Called end() with level::pml4");

	uintptr_t address = 0;

	for (level i = level::pml4; i < l; ++i) {
		uintptr_t idx = index(i) +
			((i == l) ? 1 : 0);
		address |= idx << (12 + 9*(3-unsigned(i)));
	}

	// canonicalize the address
	if (address & (1ull<<47))
		address |= (0xffffull<<48);

	return address;
}

level
page_table::iterator::align() const
{
	for (int i = 4; i > 0; --i)
		if (m_index[i-1]) return level(i);
	return level::pml4;
}

page_table::level
page_table::iterator::depth() const
{
	for (level i = level::pml4; i < level::page; ++i)
		if (!(entry(i) & 1)) return i;
	return level::pt;
}

void
page_table::iterator::next(level l)
{
	kassert(l != level::pml4, "Called next() with level::pml4");
	kassert(l <= level::page, "Called next() with too deep level");

	for (level i = l; i < level::page; ++i)
		index(i) = 0;

	while (++index(--l) == 512) {
		kassert(to_un(l), "iterator ran off the end of memory");
		index(l) = 0;
	}
}

bool
page_table::iterator::operator!=(const iterator &o) const
{
	for (unsigned i = 0; i < D; ++i)
		if (o.m_index[i] != m_index[i])
			return true;

	return o.m_table[0] != m_table[0];
}

bool
page_table::iterator::check_table(level l) const
{
	// We're only dealing with D levels of paging, and
	// there must always be a PML4.
	if (l == level::pml4 || l > level::pt)
		return l == level::pml4;

	uint64_t parent = entry(l - 1);
	if (parent & 1) {
		table(l) = reinterpret_cast<page_table*>(page_offset | (parent & ~0xfffull));
		return true;
	}
	return false;
}

void
page_table::iterator::ensure_table(level l)
{
	// We're only dealing with D levels of paging, and
	// there must always be a PML4.
	if (l == level::pml4 || l > level::pt) return;
	if (check_table(l)) return;

	page_table *table = page_table::get_table_page();
	uintptr_t phys = reinterpret_cast<uintptr_t>(table) & ~page_offset;

	uint64_t &parent = entry(l - 1);
	flag flags = table_flags;
	if (m_index[0] < memory::pml4e_kernel)
		flags |= flag::user;

	m_table[unsigned(l)] = table;
	parent = (phys & ~0xfffull) | flags;
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
}

page_table *
page_table::get(int i, uint16_t *flags) const
{
	uint64_t entry = entries[i];

	if ((entry & 0x1) == 0)
		return nullptr;

	if (flags)
		*flags = entry & 0xfffull;

	return reinterpret_cast<page_table *>((entry & ~0xfffull) + page_offset);
}

void
page_table::set(int i, page_table *p, uint16_t flags)
{
	entries[i] =
		(reinterpret_cast<uint64_t>(p) - page_offset) |
		(flags & 0xfff);
}

struct free_page_header { free_page_header *next; };

page_table *
page_table::get_table_page()
{
	if (!s_cache_count)
		fill_table_page_cache();

	free_page_header *page = s_page_cache;
	s_page_cache = s_page_cache->next;
	--s_cache_count;

	kutil::memset(page, 0, memory::frame_size);
	return reinterpret_cast<page_table*>(page);
}

void
page_table::free_table_page(page_table *pt)
{
	free_page_header *page =
		reinterpret_cast<free_page_header*>(pt);
	page->next = s_page_cache;
	s_page_cache = page->next;
	++s_cache_count;
}

void
page_table::fill_table_page_cache()
{
	constexpr size_t min_pages = 16;

	frame_allocator &fa = frame_allocator::get();
	while (s_cache_count < min_pages) {
		uintptr_t phys = 0;
		size_t n = fa.allocate(min_pages - s_cache_count, &phys);

		free_page_header *start =
			memory::to_virtual<free_page_header>(phys);

		for (int i = 0; i < n - 1; ++i)
			kutil::offset_pointer(start, i * memory::frame_size)
				->next = kutil::offset_pointer(start, (i+1) * memory::frame_size);

		free_page_header *end =
			kutil::offset_pointer(start, (n-1) * memory::frame_size);

		end->next = s_page_cache;
		s_page_cache = start;
		s_cache_count += n;
	}
}

void
page_table::free(page_table::level l)
{
	unsigned last = l == level::pml4
		? memory::pml4e_kernel
		: memory::table_entries;

	frame_allocator &fa = frame_allocator::get();
	for (unsigned i = 0; i < last; ++i) {
		if (!is_present(i)) continue;
		if (is_page(l, i)) {
			size_t count = memory::page_count(entry_sizes[unsigned(l)]);
			fa.free(entries[i] & ~0xfffull, count);
		} else {
			get(i)->free(l + 1);
		}
	}

	free_table_page(this);
}

void
page_table::dump(page_table::level lvl, bool recurse)
{
	console *cons = console::get();

	cons->printf("\nLevel %d page table @ %lx:\n", lvl, this);
	for (int i=0; i<memory::table_entries; ++i) {
		uint64_t ent = entries[i];

		if ((ent & 0x1) == 0)
			cons->printf("  %3d: %016lx   NOT PRESENT\n", i, ent);

		else if ((lvl == level::pdp || lvl == level::pd) && (ent & 0x80) == 0x80)
			cons->printf("  %3d: %016lx -> Large page at    %016lx\n", i, ent, ent & ~0xfffull);

		else if (lvl == level::pt)
			cons->printf("  %3d: %016lx -> Page at          %016lx\n", i, ent, ent & ~0xfffull);

		else
			cons->printf("  %3d: %016lx -> Level %d table at %016lx\n",
					i, ent, deeper(lvl), (ent & ~0xfffull) + page_offset);
	}

	if (lvl != level::pt && recurse) {
		for (int i=0; i<=memory::table_entries; ++i) {
			if (is_large_page(lvl, i))
				continue;

			page_table *next = get(i);
			if (next)
				next->dump(deeper(lvl), true);
		}
	}
}
