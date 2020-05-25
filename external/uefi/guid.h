#pragma once
#ifndef _uefi_guid_h_
#define _uefi_guid_h_

// This Source Code Form is part of the j6-uefi-headers project and is subject
// to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <stdint.h>

namespace uefi {

struct guid
{
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];

	inline bool operator==(const guid &other) const {
		return reinterpret_cast<const uint64_t*>(this)[0] == reinterpret_cast<const uint64_t*>(&other)[0]
			&& reinterpret_cast<const uint64_t*>(this)[1] == reinterpret_cast<const uint64_t*>(&other)[1];
	}
};

} // namespace uefi

#endif
