/// \file fs.h
/// Definitions for dealing with UEFI's disk access functions
#pragma once

#include <uefi/types.h>
#include "counted.h"

namespace uefi {
	struct boot_services;
namespace protos {
	struct file;
}}

namespace boot {
namespace fs {

/// A file or directory in a filesystem.
class file
{
public:
	file(file &&o);
	file(file &o);
	~file();

	/// Open another file or directory, relative to this one.
	/// \arg path  Relative path to the target file from this one
	file open(const wchar_t *path);

	/// Load the contents of this file into memory.
	/// \returns       A buffer describing the loaded memory. The
	///                memory will be page-aligned.
	buffer load();

private:
	friend file get_boot_volume(uefi::handle, uefi::boot_services*);

	file(uefi::protos::file *f);

	uefi::protos::file *m_file;
	uefi::boot_services *m_bs;
};

/// Get the filesystem this loader was loaded from.
/// \returns  A `file` object representing the root directory of the volume
file get_boot_volume(uefi::handle image, uefi::boot_services *bs);

} // namespace fs
} // namespace boot
