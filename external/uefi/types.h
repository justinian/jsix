#pragma once
#ifndef _uefi_types_h_
#define _uefi_types_h_

// This Source Code Form is part of the j6-uefi-headers project and is subject
// to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <stdint.h>

namespace uefi {

using handle = void *;


//
// Error and status code definitions
//
constexpr uint64_t error_bit = 0x8000000000000000ull; 
constexpr uint64_t make_error(uint64_t e) { return e|error_bit; }

enum class status : uint64_t
{
	success = 0,

#define STATUS_WARNING(name, num) name = num,
#define STATUS_ERROR(name, num) name = make_error(num),
#include "uefi/errors.inc"
#undef STATUS_WARNING
#undef STATUS_ERROR
};

inline bool is_error(status s) { return static_cast<uint64_t>(s) & error_bit; }
inline bool is_warning(status s) { return !is_error(s) && s != status::success; }


//
// Time defitions
//
struct time
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t _pad0;
	uint32_t nanosecond;
	int16_t time_zone;
	uint8_t daylight;
	uint8_t _pad1;
};


//
// Memory and allocation defitions
//
enum class memory_type : uint32_t
{
	reserved,
	loader_code,
	loader_data,
	boot_services_code,
	boot_services_data,
	runtime_services_code,
	runtime_services_data,
	conventional_memory,
	unusable_memory,
	acpi_reclaim_memory,
	acpi_memory_nvs,
	memory_mapped_io,
	memory_mapped_io_port_space,
	pal_code,
	persistent_memory,
	max_memory_type
};

enum class allocate_type : uint32_t
{
	any_pages,
	max_address,
	address
};

struct memory_descriptor
{
	memory_type type;
	uintptr_t physical_start;
	uintptr_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
};


//
// Event handling defitions
//
using event = void *;

enum class evt : uint32_t
{
	notify_wait                   = 0x00000100,
	notify_signal                 = 0x00000200,

	signal_exit_boot_services     = 0x00000201,
	signal_virtual_address_change = 0x60000202,

	runtime                       = 0x40000000,
	timer                         = 0x80000000
};

enum class tpl : uint64_t
{
	application = 4,
	callback    = 8,
	notify      = 16,
	high_level  = 31
};

using event_notify = void (*)(event, void*);


//
// File IO defitions
//
struct file_io_token
{
	event event;
	status status;
	uint64_t buffer_size;
	void *buffer;
};

enum class file_mode : uint64_t
{
	read   = 0x0000000000000001ull,
	write  = 0x0000000000000002ull,

	create = 0x8000000000000000ull
};

enum class file_attr : uint64_t
{
	none      = 0,
	read_only = 0x0000000000000001ull,
	hidden    = 0x0000000000000002ull,
	system    = 0x0000000000000004ull,
	reserved  = 0x0000000000000008ull,
	directory = 0x0000000000000010ull,
	archive   = 0x0000000000000020ull
};

} // namespace uefi

#endif
