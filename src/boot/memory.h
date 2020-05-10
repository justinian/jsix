#pragma once
#include <uefi/boot_services.h>
#include <uefi/runtime_services.h>
#include <stdint.h>
#include "kernel_args.h"

namespace boot {
namespace memory {

constexpr size_t page_size = 0x1000;

constexpr uefi::memory_type args_type   = static_cast<uefi::memory_type>(0x80000000);
constexpr uefi::memory_type module_type = static_cast<uefi::memory_type>(0x80000001);
constexpr uefi::memory_type kernel_type = static_cast<uefi::memory_type>(0x80000002);
constexpr uefi::memory_type table_type  = static_cast<uefi::memory_type>(0x80000003);

template <typename T, typename S>
static T* offset_ptr(S* input, ptrdiff_t offset) {
	return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(input) + offset);
}

inline constexpr size_t bytes_to_pages(size_t bytes) {
	return ((bytes - 1) / page_size) + 1;
}

void init_pointer_fixup(
	uefi::boot_services *bs,
	uefi::runtime_services *rs);

void mark_pointer_fixup(void **p);

kernel::args::header * allocate_args_structure(uefi::boot_services *bs, size_t max_modules);

template <typename T>
class offset_iterator
{
	T* m_t;
	size_t m_off;
public:
	offset_iterator(T* t, size_t offset=0) : m_t(t), m_off(offset) {}

	T* operator++() { m_t = offset_ptr<T>(m_t, m_off); return m_t; }
	T* operator++(int) { T* tmp = m_t; operator++(); return tmp; }
	bool operator==(T* p) { return p == m_t; }

	T* operator*() const { return m_t; }
	operator T*() const { return m_t; }
	T* operator->() const { return m_t; }
};

struct efi_mem_map {
	size_t length;
	size_t size;
	size_t key;
	uint32_t version;
	uefi::memory_descriptor *entries;

	inline size_t num_entries() const { return length / size; }

	inline uefi::memory_descriptor * operator[](size_t i) {
		size_t offset = i * size;
		if (offset > length) return nullptr;
		return offset_ptr<uefi::memory_descriptor>(entries, offset);
	}

	offset_iterator<uefi::memory_descriptor> begin() { return offset_iterator<uefi::memory_descriptor>(entries, size); }
	offset_iterator<uefi::memory_descriptor> end() { return offset_ptr<uefi::memory_descriptor>(entries, length); }
};

efi_mem_map get_mappings(uefi::boot_services *bs);

enum class memory_type
{
	free,
	loader_used,
	system_used
};

/*
EFI_STATUS memory_get_map_length(EFI_BOOT_SERVICES *bootsvc, size_t *size);
EFI_STATUS memory_get_map(EFI_BOOT_SERVICES *bootsvc, struct memory_map *map);
EFI_STATUS memory_dump_map(struct memory_map *map);

void memory_virtualize(EFI_RUNTIME_SERVICES *runsvc, struct memory_map *map);
*/

} // namespace boot
} // namespace memory
