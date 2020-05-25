#pragma once
#ifndef _uefi_tables_h_
#define _uefi_tables_h_

// This Source Code Form is part of the j6-uefi-headers project and is subject
// to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <stdint.h>
#include <uefi/guid.h>
#include <uefi/types.h>

namespace uefi {

struct runtime_services;
struct boot_services;
namespace protos {
	struct simple_text_input;
	struct simple_text_output;
}

struct table_header
{
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
};

struct configuration_table
{
	guid vendor_guid;
	void *vendor_table;
};

struct system_table
{
	table_header header;

	char16_t *firmware_vendor;
	uint32_t firmware_revision;

	handle console_in_handle;
	protos::simple_text_input *con_in;
	handle console_out_handle;
	protos::simple_text_output *con_out;
	handle standard_error_handle;
	protos::simple_text_output *std_err;

	runtime_services *runtime_services;
	boot_services *boot_services;

	unsigned int number_of_table_entries;
	configuration_table *configuration_table;
};

constexpr uint32_t make_system_table_revision(int major, int minor) {
	return (major << 16) | minor;
}

constexpr uint64_t system_table_signature = 0x5453595320494249ull;
constexpr uint32_t system_table_revision = make_system_table_revision(2, 70);
constexpr uint32_t specification_revision = system_table_revision;

namespace vendor_guids {
	constexpr guid acpi1{ 0xeb9d2d30,0x2d88,0x11d3,{0x9a,0x16,0x00,0x90,0x27,0x3f,0xc1,0x4d} };
	constexpr guid acpi2{ 0x8868e871,0xe4f1,0x11d3,{0xbc,0x22,0x00,0x80,0xc7,0x3c,0x88,0x81} };
} // namespace vendor_guids
} // namespace uefi

#endif
