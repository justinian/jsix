#pragma once
/// \file block_device.h
/// Interface definition for block devices
#include <stddef.h>

/// Interface for block devices
class block_device
{
public:
	virtual size_t read(size_t offset, size_t length, void *buffer) = 0;
};
