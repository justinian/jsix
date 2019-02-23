#include "kutil/address_manager.h"
#include "kutil/assert.h"

namespace kutil {

address_manager::address_manager(uintptr_t start, size_t length)
{
	uintptr_t p = start;
	unsigned size = size_max;

	while (size >= size_min) {
		size_t chunk_size = 1ull << size;

		while (p + chunk_size <= start + length) {
			region_node *r = m_alloc.pop();
			r->size = size_max;
			r->address = p;

			free_bucket(size).sorted_insert(r);
			p += chunk_size;
		}

		size--;
	}
}

address_manager::region_node *
address_manager::split(region_node *reg)
{
	region_node *other = m_alloc.pop();

	other->size = --reg->size;
	other->address = reg->address + (1ull << reg->size);
	return other;
}

address_manager::region_node *
address_manager::find(uintptr_t p, bool used)
{
	for (unsigned size = size_max; size >= size_min; --size) {
		auto &list = used ? used_bucket(size) : free_bucket(size);
		for (auto *f : list) {
			if (f->address < p) continue;
			if (f->address == p) return f;
			break;
		}
	}
	return nullptr;
}

uintptr_t
address_manager::allocate(size_t length)
{
	unsigned size = size_min;
	while ((1ull << size) < length)
		if (size++ == size_max)
			return 0;

	unsigned request = size;
	while (free_bucket(request).empty())
		if (request++ == size_max)
			return 0;

	region_node *r = nullptr;
	while (request > size) {
		r = free_bucket(request).pop_front();
		region_node *n = split(r);
		request--;

		free_bucket(request).sorted_insert(n);
		if (request != size)
			free_bucket(request).sorted_insert(r);
	}

	if (r == nullptr) r = free_bucket(size).pop_front();
	used_bucket(size).sorted_insert(r);

	return r->address;
}

uintptr_t
address_manager::mark(uintptr_t start, size_t length)
{
	uintptr_t end = start + length;
	region_node *found = nullptr;

	for (unsigned i = size_max; i >= size_min && !found; --i) {
		for (auto *r : free_bucket(i)) {
			if (start >= r->address && end <= r->end()) {
				found = r;
				break;
			}
		}
	}

	kassert(found, "address_manager::mark called for unknown region");
	if (!found)
		return 0;

	while (found->size > size_min) {
		// Split if the request fits in the second half
		if (start >= found->half()) {
			region_node *other = split(found);
			free_bucket(found->size).sorted_insert(found);
			found = other;
		}

		// Split if the request fits in the first half
		else if (start + length < found->half()) {
			region_node *other = split(found);
			free_bucket(other->size).sorted_insert(other);
		}

		// If neither, we've split as much as possible
		else
			break;
	}

	used_bucket(found->size).sorted_insert(found);
	return found->address;
}

address_manager::region_node *
address_manager::get_buddy(region_node *r)
{
	region_node *b = r->elder() ? r->next() : r->prev();
	if (b && b->address == r->buddy())
		return b;
	return nullptr;
}

void
address_manager::free(uintptr_t p)
{
	region_node *found = find(p, true);

	kassert(found, "address_manager::free called for unknown region");
	if (!found)
		return;

	used_bucket(found->size).remove(found);
	free_bucket(found->size).sorted_insert(found);

	while (auto *bud = get_buddy(found)) {
		region_node *eld = found->elder() ? found : bud;
		region_node *other = found->elder() ? bud : found;

		free_bucket(other->size).remove(other);
		m_alloc.push(other);

		free_bucket(eld->size).remove(eld);
		eld->size++;
		free_bucket(eld->size).sorted_insert(eld);

		found = eld;
	}
}

} // namespace kutil
