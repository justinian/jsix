#pragma once
#ifndef _uefi_runtime_services_h_
#define _uefi_runtime_services_h_

// This Source Code Form is part of the j6-uefi-headers project and is subject
// to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <stdint.h>
#include <uefi/tables.h>

namespace uefi {
namespace rs_impl {
	using convert_pointer = uefi::status (*)(uint64_t, void **);
	using set_virtual_address_map = uefi::status (*)(size_t, size_t, uint32_t, memory_descriptor *);
}

struct runtime_services {
	static constexpr uint64_t signature = 0x56524553544e5552ull;

	table_header header;

	// Time Services
	void *get_time;
	void *set_time;
	void *get_wakeup_time;
	void *set_wakeup_time;

	// Virtual Memory Services
	rs_impl::set_virtual_address_map set_virtual_address_map;
	rs_impl::convert_pointer convert_pointer;

	// Variable Services
	void *get_variable;
	void *get_next_variable_name;
	void *set_variable;

	// Miscellaneous Services
	void *get_next_high_monotonic_count;
	void *reset_system;

	// UEFI 2.0 Capsule Services
	void *update_capsule;
	void *query_capsule_capabilities;

	// Miscellaneous UEFI 2.0 Service
	void *query_variable_info;
};

} // namespace uefi
#endif
