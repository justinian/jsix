#pragma once
#include <stdint.h>
#include "kutil/enum_bitfields.h"

namespace initrd {


enum class disk_flags : uint16_t
{
};

struct disk_header
{
	uint16_t file_count;
	disk_flags flags;
	uint32_t length;
	uint8_t checksum;
	uint8_t reserved[3];
} __attribute__ ((packed));


enum class file_flags : uint16_t
{
	executable = 0x01,
	symbols    = 0x02
};

struct file_header
{
	uint32_t offset;
	uint32_t length;
	uint16_t name_offset;
	file_flags flags;
} __attribute__ ((packed));

} // namepsace initrd

IS_BITFIELD(initrd::disk_flags);
IS_BITFIELD(initrd::file_flags);
