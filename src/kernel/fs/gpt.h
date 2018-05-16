#pragma once
/// \file gpt.h
/// Definitions for dealing with GUID Partition Tables
#include "block_device.h"

namespace fs {


class partition :
	public block_device
{
public:
	/// Constructor.
	/// \arg parent  The block device this partition is a part of
	/// \arg start   The starting offset in bytes from the start of the parent
	/// \arg lenght  The length in bytes of this partition
	partition(block_device *parent, size_t start, size_t length);

	/// Read bytes from the partition.
	/// \arg offset  The offset in bytes at which to start reading
	/// \arg length  The number of bytes to read
	/// \arg buffer  [out] Data is read into this buffer
	/// \returns     The number of bytes read
	virtual size_t read(size_t offset, size_t length, void *buffer);

	/// Find partitions on a block device and add them to the device manager
	/// \arg device  The device to search for partitions
	/// \returns     The number of partitions found
	static unsigned load(block_device *device);

private:
	block_device *m_parent;
	size_t m_start;
	size_t m_length;
};

} // namespace fs
