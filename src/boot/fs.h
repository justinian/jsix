#pragma once

#include <uefi/types.h>
#include <uefi/boot_services.h>
#include <uefi/protos/file.h>

namespace boot {
namespace fs {

class file
{
public:
	file(uefi::protos::file *f);
	file(file &&o);

	file open(const wchar_t *path);

private:
	uefi::protos::file *m_file;
};

file get_boot_volume(uefi::handle image, uefi::boot_services *bs);

} // namespace fs
} // namespace boot
