#pragma once

#include <uefi/types.h>
#include <uefi/boot_services.h>
#include <uefi/protos/file.h>

namespace boot {
namespace fs {

class file
{
public:
	file(file &&o);
	file(file &o);
	~file();

	file open(const wchar_t *path);
	void * load(
		size_t *out_size,
		uefi::memory_type mem_type = uefi::memory_type::loader_data);

private:
	friend file get_boot_volume(uefi::handle, uefi::boot_services*);

	file(uefi::protos::file *f, uefi::boot_services *bs);

	uefi::protos::file *m_file;
	uefi::boot_services *m_bs;
};

file get_boot_volume(uefi::handle image, uefi::boot_services *bs);

} // namespace fs
} // namespace boot
