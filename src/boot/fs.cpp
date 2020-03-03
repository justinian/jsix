#include <uefi/types.h>
#include <uefi/protos/file.h>
#include <uefi/protos/file_info.h>
#include <uefi/protos/loaded_image.h>
#include <uefi/protos/simple_file_system.h>

#include "fs.h"
#include "console.h"
#include "error.h"
#include "memory.h"

namespace boot {
namespace fs {

file::file(uefi::protos::file *f, uefi::boot_services *bs) :
	m_file(f),
	m_bs(bs)
{
}

file::file(file &o) :
	m_file(o.m_file),
	m_bs(o.m_bs)
{
	o.m_file = nullptr;
}

file::file(file &&o) :
	m_file(o.m_file),
	m_bs(o.m_bs)
{
	o.m_file = nullptr;
}

file::~file()
{
	if (m_file)
		m_file->close();
}

file
file::open(const wchar_t *path)
{
	uefi::protos::file *fh = nullptr;

	try_or_raise(
		m_file->open(&fh, path, uefi::file_mode::read, uefi::file_attr::none),
		L"Could not open relative path to file");

	return file(fh, m_bs);
}

void *
file::load(size_t *out_size)
{
	uint8_t buffer[sizeof(uefi::protos::file_info) + 100];
	size_t size = sizeof(buffer);
	uefi::guid info_guid = uefi::protos::file_info::guid;

	try_or_raise(
		m_file->get_info(&info_guid, &size, &buffer),
		L"Could not get file info");

	uefi::protos::file_info *info =
		reinterpret_cast<uefi::protos::file_info*>(&buffer);

	size_t pages = memory::bytes_to_pages(info->file_size);
	void *data = nullptr;
	try_or_raise(
		m_bs->allocate_pages(
			uefi::allocate_type::any_pages,
			uefi::memory_type::loader_data,
			pages, &data),
		L"Could not allocate pages to load file");

	size = info->file_size;
	try_or_raise(
		m_file->read(&size, data),
		L"Could not read from file");

	*out_size = size;
	return data;
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

	return file(f, bs);
}

} // namespace fs
} // namespace boot

