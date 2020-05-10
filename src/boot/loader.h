#pragma once

namespace boot {
namespace loader {

struct loaded_elf
{
	void *data;
	uintptr_t vaddr;
	uintptr_t entrypoint;
};

loaded_elf load(const void *data, size_t size, uefi::boot_services *bs);

} // namespace loader
} // namespace boot
