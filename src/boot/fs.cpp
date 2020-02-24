#include <uefi/types.h>
#include <uefi/protos/loaded_image.h>
#include <uefi/protos/simple_file_system.h>

#include "fs.h"
#include "console.h"
#include "error.h"

namespace boot {
namespace fs {

file::file(uefi::protos::file *f) :
	m_file(f)
{
}

file::file(file &&o) :
	m_file(o.m_file)
{
	o.m_file = nullptr;
}

file
file::open(const wchar_t *path)
{
	uefi::protos::file *fh = nullptr;

	try_or_raise(
		m_file->open(&fh, path, uefi::file_mode::read, uefi::file_attr::none),
		L"Could not open relative path to file");

	return file(fh);
}

file
get_boot_volume(uefi::handle image, uefi::boot_services *bs)
{
	status_line status(L"Looking up boot volume");

	const uefi::guid le_guid = uefi::protos::loaded_image::guid;
	uefi::protos::loaded_image *loaded_image = nullptr;

	try_or_raise(
		bs->handle_protocol(image, &le_guid,
			reinterpret_cast<void**>(&loaded_image)),
		L"Could not find currently running UEFI loaded image");

	const uefi::guid sfs_guid = uefi::protos::simple_file_system::guid;
	uefi::protos::simple_file_system *fs;
	try_or_raise(
		bs->handle_protocol(loaded_image->device_handle, &sfs_guid,
			reinterpret_cast<void**>(&fs)),
		L"Could not find filesystem protocol for boot volume");

	uefi::protos::file *f;
	try_or_raise(
		fs->open_volume(&f),
		L"Could not open the boot volume");

	return file(f);
}

} // namespace fs
} // namespace boot

